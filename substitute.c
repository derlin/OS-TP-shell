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

#define isValidStart(c) (isalpha(c) || (c) == '_')
#define isValid(c)      (isalpha(c) || isdigit(c) || (c) == '_')

#define MAX_ARG_SIZE    1024

char * extractVar(char * src, char * p, char * varname)
{
    char * start = p, *end;
    BOOLEAN bracket = *p == '{';
    if(bracket)
    {
        start++;
        p++;
    }   // skip "{"

    if(!isValidStart(*p))
    {
        fprintf( stderr, "parse error: invalid character '%c'\n", *p);
        return NULL;
    }

    while(*p && isValid(*p))
        p++;

    if(bracket && *p != '}')
    {
        fprintf( stderr, "parse error, unbalanced '}'\n");
        return NULL;
    }

    end = p;
    memcpy(varname, start, end - start);
    varname[ end - start ] = 0;

    return p;
}

// -------------------------------------

int substitute(char * src, char * dest, int len)   // TODO : size of dest ?
{
    char * sp = src, *dp = dest;   // pointers

    while(*sp)
    {
        if(dp - dest >= len)
        {   // avoid segfaults
            fprintf(stderr, "out of memory error: expansion too long.\n");
            return SUBST_OUT_OF_MEM;
        }

        /* escape char */
        if(*sp == '\\' && *(sp + 1))
        {
            sp++;   // skip '\'
            *(dp) = *(sp++);   // copy and skip the next one
        }
        /* potential variable */
        else if(*sp == '$')
        {
            char * subst;
            char varname[ 40 ];
            char pid[ 5 ];   // for the pid in case of $$

            sp++;   // ignore '$'

            if(*sp && *sp == '$' && (*(sp + 1) == 0 || isspace(*(sp+1))))
            {
                // we have a $$, replace $$ by pid
                sprintf(varname, "$$");
                sprintf(pid, "%d", getpid());
                subst = pid;
                sp++;   // skip second $
            }
            else
            {
                // normal case: parse variable name
                sp = extractVar(src, sp, varname);
                if(sp == NULL)
                {
                    // parse error: don't process further
                    *dp = 0; // properly terminate the string
                    return SUBST_PARSE_ERROR;
                }

                if(*sp == '}') sp++;   // skip remaining bracket if ${..}
                subst = EVget(varname);
            }

            if(subst == NULL)
            {
                // variable with no value
                fprintf( stderr, "warning: undefined variable : %s\n", varname);
            }
            else
            {
                // copy value in dest
                int len = strlen(subst);
                memcpy(dp, subst, len);
                dp += len;
            }
        }
        else
        {
            // normal case: copy char
            *(dp++) = *(sp++);
        }
    }

    *dp = 0;

    return SUBST_OK;
}
// -------------------------------------

