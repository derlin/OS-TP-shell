/**
 * @file preproc.c
 * 
 * @date 27 Nov 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 * PURPOSE
 * =======
 *  - v1: manage the history + hX commands and do the variable expansion. Problem: by doing the expansion here,
 *      something like: >a=A; echo $a would not work...
 *  - v2: only keep the history up to date (+ hX commands). The expansion is made in the parser.
 *
 * HISTORY
 * =======
 *
 * The history is made up of the last ten valid lines entered, i.e.
 * passed to the parser. A line is considered invalid if it contained a hX command where the X exceeds
 * the number of history entries.
 *
 *
 * CHOICES, BUGS AND SPECIAL CASES
 * ===============================
 *
 * - If the input is a hX with X > 10, it is not considered as a hX command, so it will be left untouched.
 * - If we use h4 and there is no such history, a warning message is printed to stderr and the whole
 *   input is discarded.
 * - the history command is also stored into history, as well as "repeated commands" and hX expansions.
 * - the history always stores the whole input, unparsed. So the variables are not expanded.
 * - special case/bug: this implementation does not cope with multiple lines input:
 *      > echo "lala
 *      lulu"
 *      lala
 *      lulu
 *
 *      > history
 *        1: history
 *        2: lulu"
 *        3: echo "lala
 *
 * - the command "history" is stored before its execution, so the first line will always be "1: history"...
 *
 */

#include "preproc.h"
#include "defs.h"
#include "environ.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

//--------------------------------------------------------
// history
#define IncModulo(x)    ((x+1)%HISTORY_SIZE)
#define DecModulo(x)    ((x+HISTORY_SIZE-1)%HISTORY_SIZE)

BOOLEAN subst_history(char * src, char * dest);
static char * history[HISTORY_SIZE] = { 0 };
static int top = 0;

//--------------------------------------------------------
// preproc
#define MAX_CMD 1024

static char cur_cmd[MAX_CMD] = { 0 };
static int cur_index = 0;

/* init cur_cmd to { 0 } */
inline void reset_cmd()
{
    int i = 0;
    for(; i < MAX_CMD; i++)
    {
        cur_cmd[i] = 0;
    }
}

/**
 * @return the next available char
 */
char get_char()
{
    char ret = cur_cmd[cur_index++];

    if(ret == 0)   // get new input
    {
        int i = 0;
        reset_cmd();
        char temp[MAX_CMD] =
        { 0 };   // init not mandatory, but we never know

        while(1)
        {
            char c = getchar();
            temp[i++] = c;

            if(c == '\n')
            {
                temp[i] = 0;
                break;
            }

            if(i >= MAX_CMD)   // avoid buffer overflows
            {
                fprintf( stderr, "error: input too long.\n");
                exit(1);
            }
        }

        // reset
        cur_index = 0;
        // replace hX by the matching history item
        if(subst_history(temp, cur_cmd))
        {
            // add to history only if no errors
            add_history(cur_cmd);
        }
        else
        {
            // error => replace input by an empty cmd
            // so that the shell will redisplay the prompt
            strcpy(cur_cmd, "\n");
        }

        ret = cur_cmd[cur_index++];
    }

    return ret;
}

/**
 * put back a char to the stream
 * @param c the char to put back.
 */
void un_getc(char c)
{
    assert(cur_index >= 1);
    cur_cmd[--cur_index] = c;
}
//--------------------------------------------------------

/**
 * add a command to the history
 * @param cmd the command to add
 */
void add_history(char * cmd)
{
    if(history[top] != 0) free(history[top]);
    if((history[top] = (char *) malloc(strlen(cmd) + 1)) == NULL)
    {
        fprintf( stderr, "error: malloc: cannot add %s to history.\n", cmd);
    }
    else
    {
        strcpy(history[top], cmd);
        top = IncModulo(top);
    }
}

/**
 * print the current history entries. The last cmd is at index 1.
 */
void print_history()
{
    int index = DecModulo(top);
    if(history[index] == NULL)
    {
        // should never happen.
        printf("History empty...\n");
    }
    else
    {
        int cnt;
        for(cnt = 1; cnt <= 10; cnt++)
        {
            if(history[index] == 0) break;   // security
            printf(" %2d: %s", cnt, history[index]);
            index = DecModulo(index);
        }
    }
}

/**
 * get an history entry.
 * @param i the entry index, between 1 and 10
 * @return either the matching command, or NULL if it does not exist.
 */
char * get_history_at(int i)
{
    i = (top + HISTORY_SIZE - i) % HISTORY_SIZE;
    return history[i];   // either NULL or cmd
}

// ------------------------------------------

#define Is_delim(c) ((c) == '&' || (c) == '|' || (c) == ';' || (c) == '\n')

/**
 * check if the command is a valid hX
 * @param data the whole input string
 * @param start a pointer to the start of a command
 * @param hist_num the parsed history entry number, if any
 * @return 1 if a hX command was found, 0 otherwise
 */
BOOLEAN is_hcmd(char * data, char ** start, int * hist_num)
{
    char * ph;
    ph = data;

    while(isspace(*ph))
        ph++;
    if(*ph == 0 || *ph != 'h') return 0;

    char * pn = ph + 1;

    if(*pn && isdigit(*pn))
    {
        char * pnn = pn + 1;
        if(*pnn == 0 || (*pnn && (isspace(*pnn) || Is_delim(*pnn))) || (*pn == '1' && *pnn == '0'))
        {
            int hist = *pn - '0';
            if(*pnn == '0') hist = 10;

            *hist_num = hist;
            *start = ph;

            return TRUE;
        }
    }
    return FALSE;
}

/*
 * substitute hX be the corresponding command.
 * should support cases like:
 *    > h1
 *    > echo lala; h2;h4
 *    > h3 | grep B
 *
 * so the rules are:
 * - hX is either the first chars, or is preceded by a delim + any number of spaces.
 * - every space, other char, etc. must be preserved.
 * @param src the original line
 * @param dest the line after substitution of all hX
 */

/**
 * substitute hX commands by the corresponding history entry.
 * @param src the whole input to parse
 * @param dest the expanded result
 * @return false if an error occurred (hX with X > number of entries), true otherwise.
 */
BOOLEAN subst_history(char * src, char * dest)
{
    int first = 1;
    char * start;
    int hist_num;

    while(*src)
    {
        if(first || Is_delim( *src ))
        {
            // copy delimiters
            if(!first) *(dest++) = *(src++);

            if(is_hcmd(src, &start, &hist_num))
            {
                // copy spaces
                while(src != start)
                {
                    *(dest++) = *(src++);
                }

                char * subst = get_history_at(hist_num);
                if(subst == 0)
                {
                    fprintf(stderr, "error: no history at %d\n", hist_num);
                    return FALSE;
                }
                else
                {
                    strcpy(dest, subst);
                    dest += strlen(subst) - 1;   // remove \n
                    src += 2;
                    if(hist_num == 10) src++;
                }

                if(*src == 0) break;
            }
        }
        first = Is_delim(*src);
        *(dest++) = *(src++);
    }

    dest = 0;
    return TRUE;
}

