/**
 * @file substitute.c
 * 
 * @date 13 Dec 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */

#include "substitute.h"
#include "environ.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

// -------------------------------------


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
