/**
 * @file substitute.c
 * 
 * @date 13 Dec 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 *
 * Do the variable expansion.
 *
 * FUNCTIONALITIES
 * ===============
 *  a variable is either $varname or ${varname}. The special $$ is expanded with
 *  the current shell process id.
 *  To escape a variable, use \$. Note that the '\' escape char is always stripped from the input.
 *
 * if a variable is undefined, it will be replaced by "".
 *
 *  EXAMPLES
 *  ========
 *
 *      > a=A; b=B; echo ${a}${b}\$b $a
 *      AB$b A
 *
 *      > echo $$
 *      2345
 *
 *      > echo \$\$
 *      $$
 *
 *      >echo -$undef-
 *      warning: undefined variable : undef
 *      --
 *
 * NOTES
 * =====
 *
 *  -   the space cannot be escaped, since the parser will treat the former as a delimiter. Thus,
 *          > echo 'lala\ '
 *      will result in:
 *          lala\
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

#define MAX_VARNAME_LEN    40
#
/**
 * Extract the variable name.
 * @param src the whole input string
 * @param p a pointer to the beginning of the potential variable name (the char following '$')
 * @param varname where to store the variable name
 * @return a pointer to the end of the variable name, or NULL if an error occurred.
 */
char * extractVar(char * src, char * p, char * varname)
{
    char * start = p, *end;
    BOOLEAN bracket = *p == '{';

    if(bracket)  // skip "{"
    {
        start++;
        p++;
    }

    if(!isValidStart(*p))
    {
        fprintf( stderr, "parse error: invalid character '%c'.\n", *p);
        return NULL;
    }

    while(*p && isValid(*p))
        p++;

    if(bracket && *p != '}')
    {
        fprintf( stderr, "parse error, unbalanced '}'.\n");
        return NULL;
    }

    end = p;

    if(end-start >= MAX_VARNAME_LEN)
    {
        fprintf(stderr, "error, variable name too long.\n");
        return NULL;
    }

    memcpy(varname, start, end - start);
    varname[ end - start ] = 0;

    return p;
}

// -------------------------------------

/**
 * expand variables.
 * @param src the input to expand
 * @param dest a buffer for the result
 * @param len the size of the buffer
 * @return 0 = 0k, 1 = buffer too short, 2 = parse error
 * Note: error messages are printed to stderr
 */
int substitute(char * src, char * dest, int len)   // TODO : size of dest ?
{
    char * sp = src, *dp = dest;   // pointers

    while(*sp)
    {
        /* avoid buffer overflows */
        if(dp - dest >= len)
        {
            fprintf(stderr, "out of memory error: expansion too long.\n");
            return SUBST_OUT_OF_MEM;
        }

        /* escape char */
        if(*sp == '\\' && *(sp + 1))
        {
            sp++;   // skip '\'
            *(dp++) = *(sp++);   // copy and skip the next one
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

