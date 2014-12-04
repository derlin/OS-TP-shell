/**
 * @file history.c
 * 
 * @date 27 Nov 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
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
        substitute( temp, cur_cmd );
        if( *cur_cmd == 0 )   // error, undefined variable found
        {
            ret = '\n';   // return empty line
        }
        else   // substitution ok
        {
            // first, make the substitution (h1 => cmd)
            strcpy( temp, cur_cmd );
            subst_history( temp, cur_cmd );
            // then, add to history
            add_history( cur_cmd );
            // TODO printf("NEW CMD: %s", cur_cmd);
            ret = cur_cmd[ cur_index++ ];
        }
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

#define Is_delim(c) ((c) == '&' || (c) == '|' || (c) == ';')

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
        if( *pnn == 0 || ( *pnn && isspace(*pnn) ) || ( *pn == '1' && *pnn == '0' ) )
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

void subst_history( char * src, char * dest )
{
    int first = 1;
    char * start;
    int hist_num;

    while( *src )
    {
        if( first || Is_delim( *src ) )
        {
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
//                    printf("subst %s\n", subst);
                    strcpy( dest, subst );
                    dest += strlen( subst ) - 1; // remove \n
                    src += 2;
                    if( hist_num == 10 ) src++;
                }

                if( *src == 0 ) break;
            }
        }

        *( dest++ ) = *( src++ );
        first = 0;
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

    int quote = 0;

    while( *p )
    {

        *( dest++ ) = *p;
        if( *p == '\\' && *( p + 1 ) ) *( dest++ ) = *(++p); // copy escaped char
        else if( *p == '"' ) quote = !quote;
        else if( !quote && *p == '$' )
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
                *destination = 0;
                break;
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

