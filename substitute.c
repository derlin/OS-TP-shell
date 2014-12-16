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
    }   // skip "{"

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


char * substitute( char * src )   // TODO : size of dest ?
{
    char temp[2048]; // buffer for the substituted string
    char * sp = src, * dp = temp;

    int substOccurred = 0;

    while( *sp )
    {

        *( dp++ ) = *sp; // copy current

        if( *sp == '\\' && *( sp + 1 ) ) // escape character
        {
            *( dp-1 ) = *(++sp); // skip escaped char and ignore the next one
        }
        else if( *sp == '$' ) // candidate to a var
        {
            char * subst;
            char varname[ 40 ];
            char pid[5]; // for the pid in case of $$

            dp--; sp++; // remove $ from dest

            if(*sp && *sp == '$' && (*(sp+1) == 0 || isspace(*(sp+1))))
            {
                // special case: $$ => pid
                sprintf(varname, "$$");
                sprintf(pid, "%d", getpid());
                subst = pid;
                sp++; // skip second $
            }
            else
            {
                // normal case: parse variable name
                sp = extractVar( src, sp, varname );
                if( sp == NULL )
                {
                    // parse error: don't process further
                    return NULL;
                }

                if( *sp == '}' ) sp++; // skip remaining bracket if ${..}
                subst = EVget( varname );
            }

            if( subst == NULL )
            {
                fprintf( stderr, "warning: undefined variable : %s\n", varname );
                //*destination = 0;
                //break;
                continue;
            }

            int len = strlen( subst );
            memcpy( dp, subst, len );
            dp += len;
            substOccurred = 1;
            continue;
        }
        sp++;
    }

    *dp = 0;

    char * ret = NULL;
    if(substOccurred){
        ret = (char *) malloc(strlen(dp) +1);
        strcpy(ret, temp);
    }

    return ret;
}
