/**
 * @file history.c
 * 
 * @date 27 Nov 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */

#include "preproc.h"
#include "defs.h"
#include "environ.h"
#include <ctype.h>

#define IncModulo(x)    ((x+1)%HSIZE)
#define DecModulo(x)    ((x+HSIZE-1)%HSIZE)
#define HSIZE   10

static char * history[ HSIZE ] =
{ 0 };
static int top = 0;

//--------------------------------------------------------
#define MAX_CMD 400
static char cur_cmd[ MAX_CMD ] =
{ 0 };
static int cur_index = 0;

char get_char()
{
    char ret = cur_cmd[cur_index++];

    if( ret == 0 )
    {
        int i = 0;
        char temp[MAX_CMD];

        while( 1 )
        {
            char c = getchar();
            temp[ i++ ] = c;

            if( c == '\n' )
            {
                temp[i] = 0;
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
        substitute(temp, cur_cmd);
        if(*cur_cmd == 0) // error, undefined variable found
        {
            ret = '\n'; // return empty line
            //*(cur_cmd+1) = 0; // next iter => start over
        }
        else
        {
            ret = cur_cmd[cur_index++];
            add_history(cur_cmd);
        }
    }



    return ret;
}

void un_getc(char c)
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
    if( history[ top ] == NULL )
    {
        printf( "History empty...\n" );

    }
    else
    {
        int i = top, cnt = 1;

        do
        {
            printf( " %2d: %s\n", cnt++, history[ i ] );
            i = DecModulo(i);

        }while( history[ i ] != 0 && i != top );
    }
}

char * get_history_at( int i )
{

    i = ( top + HSIZE - i ) % HSIZE;
    return history[ i ];   // either NULL or cmd

}

// -------------------------------------

#define isValidStart(c) (isalpha(c))
#define isValid(c)      (isalpha(c) || isdigit(c))

char * extractVar(char * src, char * p, char * varname)
{
    char * start = p, * end;
    BOOLEAN bracket = *p == '{';
    if(bracket){ start++; p++; } // skip {

    if(!isValidStart(*p))
    {
        fprintf(stderr, "error: invalid character %c\n", *p);
        return NULL;
    }

    while(*p && isValid(*p) ) p++;

    if( bracket && *p != '}')
    {
        fprintf(stderr, "error, unbalanced }\n");
        return NULL;
    }

    end = p;
    memcpy(varname, start, end-start);
    varname[end-start] = 0;

    return p;
}


void substitute(char * src, char * destination) // TODO : size of dest ?
{

    char * dest = destination;
    char * p = src;
    char varname[40];

    int quote = 0;

    while(*p)
    {
        *(dest++) = *p;
        if( *p == '\\' && *(p+1)) p++; // skip next
        else if( *p == '"') quote = !quote;
        else if(!quote && *p == '$')
        {
            dest--;


            p = extractVar(src, ++p, varname);
            if(p == NULL) break;
            if(*p == '}') p++; // skip remaining bracket if ${..}

            char * subst = EVget(varname);

            if(subst == NULL)
            {
                fprintf(stderr, "undefined variable : %s\n", varname);
                *destination = 0;
                break;
            }

            int len = strlen(subst);
            memcpy(dest, subst, len);
            dest+=len;
            continue;
        }
        p++;
    }

    *dest = 0;
}


