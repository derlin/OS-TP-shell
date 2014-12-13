/**
 * @file preproc.c
 * 
 * @date 27 Nov 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 * PURPOSE:
 * - keep the history up to date
 * - variable substitution
 *
 * HISTORY
 * =======
 *
 * The history is made up of the last ten valid lines entered, i.e.
 * passed to the parser. By invalid, we mean a line for which
 * the substitution process failed.
 *
 * If we use h4 and there is no such history, the string ("h4") is
 * left untouched and a warning message is printed to stderr.
 *
 *
 * VARIABLE SUBSTITUTION
 * =====================
 *
 * The shell supports the following:
 *   $varname
 *   ${varname}
 *
 * example:
 *  a=toto
 *  b=titi
 *
 *  echo $a ${a}X$b "${b}" \$\$  => toto totoXtiti ${b} $$
 *
 * The special variable $$ is replaced by the current process's PID.
 * Note that a variable inside double quotes is not replaced.
 *
 *
 * CHOICES
 * =======
 * We decided to do the substitution first and to ignore the input if
 * the substitution fails:
 *
 *      fmk-> echo $a
 *      undefined variable : a
 *
 * Then, the input is tested against the hX patterns, which will  be replaced by the
 * corresponding history entry. Finally, the line is added to the history.
 *
 * This means that echo $$ will be stored into history as echo <pid> and that
 * hX will never show up in history.
 *
 * Those are choices that are highly debatable (and I guess I would do
 * it differently now that I think of it...). But keep in mind that if
 * we'd want another behaviour, it would not be such a hassle: just move some lines
 * around in the get_char method !
 *
 *
 * BUGS/SPECIAL CASES
 * ==================
 * - the treatment of the "\" escape character is tricky. Should we
 *   remove it during substitution or not ?
 *   Right now, the "\" is removed and the escaped char is left untouched:
 *      fmk-> echo \\n
 *      \n
 *      fmk-> echo \$\$ \$a
 *      $$ \$a
 *  This behaviour seams correct, but the bug happens when we try to
 *  escape the double quote:
 *      fmk-> echo "\"hello\""
 *      hello""
 *   V2: get rid of the "escaping quote", which is not really useful.
 *       To escape a variable, simply write \$a !
 *
 * - the command on multiple lines are properly treated by the history:
 *
 *   fmk-> echo "lala
 *   lulu"
 *   fmk-> history
 *      1: history
 *      2: lulu"
 *      3: echo "lala
 *
 *
 * - the history command is added to the history before the command's
 *   execution. So the h1 will always be history... It could be possible to
 *   change it, but other special cases would appear (no "right" solution)
 *
 * - as previously mentioned, the variables are substituted BEFORE the
 *   command is added to history.
 */


// TODO   how to manager the escaped char ? either skip it => echo "\"hello\"" not working
//        or keep it: echo \$$$ => \$<pid>...
#include "preproc.h"
#include "defs.h"
#include "environ.h"
#include <ctype.h>
#include <unistd.h>

#define IncModulo(x)    ((x+1)%HSIZE)
#define DecModulo(x)    ((x+HSIZE-1)%HSIZE)
#define HSIZE   10

static char * history[ HSIZE ] =
{ 0 };
static int top = 0;


void subst_history( char * src, char * dest );


//--------------------------------------------------------
#define MAX_CMD 400
static char cur_cmd[ MAX_CMD ] =
{ 0 };
static int cur_index = 0;

char get_char()
{
    char ret = cur_cmd[ cur_index++ ];

    if( ret == 0 )
    {
        int i = 0;
        char temp[ MAX_CMD ];

        while( 1 )
        {
            char c = getchar();
            temp[ i++ ] = c;

            if( c == '\n' )
            {
                temp[ i ] = 0;
                break;
            }

            if( i >= MAX_CMD )
            {
                fprintf( stderr, "Aie, buffer too short (pre-parser)\n" );
                exit( 1 );
            }
        }

        // reset
        cur_index = 0;
        // substitute variables
//        substitute( temp, cur_cmd );
//        if( *cur_cmd == 0 )   // error, undefined variable found
//        {
//            ret = '\n';   // return empty line
//        }
//        else   // substitution ok
//        {
            // first, make the substitution (h1 => cmd)
//            strcpy( temp, cur_cmd );
            subst_history( temp, cur_cmd );
            // then, add to history
            add_history( cur_cmd );
            // TODO printf("NEW CMD: %s", cur_cmd);
            ret = cur_cmd[ cur_index++ ];
//        }
    }

    return ret;
}

void un_getc( char c )
{
    cur_index--;
}
//--------------------------------------------------------

void add_history( char * cmd )
{
    if( history[ top ] != 0 ) free( history[ top ] );

    if( ( history[ top ] = (char *) malloc( strlen( cmd ) + 1 ) ) == NULL )
    {
        fprintf( stderr, "Could not add cmd to history\n" );
    }
    else
    {
        strcpy( history[ top ], cmd );
        top = IncModulo(top);
    }
}

void print_history()
{
    int index = DecModulo(top);
    if( history[ index ] == NULL )
    {
        printf( "History empty...\n" );

    }
    else
    {
        int cnt;
        for(cnt = 1; cnt <= 10; cnt++)
        {
            if(history[ index ] == 0) break; // security
            printf( " %2d: %s", cnt, history[ index ] );
            index = DecModulo(index);
        }
    }
}

char * get_history_at( int i )
{

    i = ( top + HSIZE - i ) % HSIZE;
    return history[ i ];   // either NULL or cmd

}


// ------------------------------------------

#define Is_delim(c) ((c) == '&' || (c) == '|' || (c) == ';' || (c) == '\n')

int is_hcmd( char * data, char ** start, int * hist_num )
{
    char * ph;
    ph = data;

    while( isspace(*ph) )
        ph++;
    if( *ph == 0 || *ph != 'h' ) return 0;

    char * pn = ph + 1;

    if( *pn && isdigit(*pn) )
    {
        char * pnn = pn + 1;
        if( *pnn == 0 || ( *pnn && (isspace(*pnn) || Is_delim(*pnn)) ) || ( *pn == '1' && *pnn == '0' ) )
        {
            int hist = *pn - '0';
            if( *pnn == '0' ) hist = 10;
//            printf( "Found h%d at %d\n", hist, ph - data );

            *hist_num = hist;
            *start = ph;

            return 1;
        }
    }
    return 0;
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
void subst_history( char * src, char * dest )
{
    int first = 1;
    char * start;
    int hist_num;

    while( *src )
    {
        if( first || Is_delim( *src ) )
        {
            // copy delimiters
            if( !first ) *( dest++ ) = *( src++ );

            if( is_hcmd( src, &start, &hist_num ) )
            {
                // copy spaces
                while( src != start )
                {
                    *( dest++ ) = *( src++ );
                }

                char * subst = get_history_at( hist_num );
                if( subst == 0 )
                {
                    fprintf( stderr, "no hist at %d\n", hist_num );

                }
                else
                {
                    strcpy( dest, subst );
                    dest += strlen( subst ) - 1; // remove \n
                    src += 2;
                    if( hist_num == 10 ) src++;
                }

                if( *src == 0 ) break;
            }
        }
        first = Is_delim(*src);
        *( dest++ ) = *( src++ );
    }

    dest = 0;
}

// -------------------------------------
// =====================================

#define isValidStart(c) (isalpha(c))
#define isValid(c)      (isalpha(c) || isdigit(c))

char * extractVar( char * src, char * p, char * varname )
{
    char * start = p, *end;
    BOOLEAN bracket = *p == '{';
    if( bracket )
    {
        start++;
        p++;
    }   // skip {

    if( !isValidStart(*p) )
    {
        fprintf( stderr, "error: invalid character %c\n", *p );
        return NULL;
    }

    while( *p && isValid(*p) )
        p++;

    if( bracket && *p != '}' )
    {
        fprintf( stderr, "error, unbalanced }\n" );
        return NULL;
    }

    end = p;
    memcpy( varname, start, end - start );
    varname[ end - start ] = 0;

    return p;
}

void substitute( char * src, char * destination )   // TODO : size of dest ?
{

    char * dest = destination;
    char * p = src;
    char varname[ 40 ];

    while( *p )
    {

        *( dest++ ) = *p;
        if( *p == '\\' && *( p + 1 ) ) *( dest-1 ) = *(++p); // skip escaped char and ignore the next one
//        else if( *p == '"' ) quote = !quote;
//        else if( !quote && *p == '$' )
        else if( *p == '$' )
        {
            dest--; p++;
            char * subst;
            char pid[5]; // for the pid in case of $$

            if(*p && *p == '$' && (*(p+1) == 0 || isspace(*(p+1))))
            {
                // special case: $$ => pid
                sprintf(varname, "$$");
                sprintf(pid, "%d", getpid());
                subst = pid;
                p++; // skip second $
            }
            else
            {
                // normal case: check if really a variable
                // if not, error
                p = extractVar( src, p, varname );
                if( p == NULL )
                {
                    *destination = 0;
                    break;
                }
                if( *p == '}' ) p++;   // skip remaining bracket if ${..}

                subst = EVget( varname );
            }

            if( subst == NULL )
            {
                fprintf( stderr, "undefined variable : %s\n", varname );
                //*destination = 0;
                //break;
                continue;
            }

            int len = strlen( subst );
            memcpy( dest, subst, len );
            dest += len;
            continue;
        }
        p++;
    }

    *dest = 0;
}

