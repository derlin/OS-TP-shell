/**
 * @file substitute.c
 * 
 * @date 13 Dec 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */

#include "substitute.h"
#include "environ.h"
#include "parser.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define isValidStart(c) (isalpha(c))
#define isValid(c)      (isalpha(c) || isdigit(c))

#define MAX_ARG_SIZE    1024

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
    char * ret = NULL;   // return value
    char buffer[ MAX_ARG_SIZE ];   // buffer for the substituted string

    char * sp = src, *dp = buffer; // pointers

    int substOccurred = 0;

    while( *sp )
    {
        if(dp - buffer >= MAX_WORD){ // avoid segfaults
            fprintf(stderr, "Ran out of memory during substitution!");
            return NULL;
        }

        /* escape char */
        if( *sp == '\\' && *( sp + 1 ) )
        {
            sp++;   // skip '\'
            *( dp ) = *( sp++ );   // copy and skip the next one
        }
        /* potential variable */
        else if( *sp == '$' )
        {
            char * subst;
            char varname[ 40 ];
            char pid[ 5 ];   // for the pid in case of $$

            sp++;   // ignore '$'

            if( *sp && *sp == '$' && ( *( sp + 1 ) == 0 || isspace(*(sp+1)) ) )
            {
                // we have a $$, replace $$ by pid
                sprintf( varname, "$$" );
                sprintf( pid, "%d", getpid() );
                subst = pid;
                sp++;   // skip second $
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

                if( *sp == '}' ) sp++;   // skip remaining bracket if ${..}
                subst = EVget( varname );
            }

            if( subst == NULL )
            {
                // variable with no value
                fprintf( stderr, "warning: undefined variable : %s\n", varname );
            }
            else
            {
                // copy value in dest
                int len = strlen( subst );
                memcpy( dp, subst, len );
                dp += len;
                substOccurred = 1;
            }
        }
        else
        {
            // normal case: copy char
            *( dp++ ) = *( sp++ );
        }
    }

    *dp = 0;

    if( substOccurred )
    {
        ret = (char *) malloc( strlen( dp ) + 1 );
        strcpy( ret, buffer );
    }

    return ret;
}
