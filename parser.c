#include "parser.h"
#include "syserr.h"
#include "substitute.h"
#include <unistd.h>
#include <stdlib.h>

/**
 * MODIFICATIONS
 * =============
 *  - replace calls to getchar and ungetc by our custom get_char
 *    and unget_char. The parser now gets its input from the
 *    preproc, not stdin !
 *  - do the variable substitution for arguments, commands and
 *    src/dstfile
 */

/* collect and classify token */
TOKEN gettoken(char *word)
{
    enum
    {
        NEUTRAL, GTGT, INQUOTE, INWORD
    } state = NEUTRAL;
    int c;
    char *w;

    w = word;
    while((c = get_char()) != EOF)
    {
        switch(state)
        {
            case NEUTRAL:
                switch(c)
                {
                    case ';':
                        return (T_SEMI);
                    case '&':
                        return (T_AMP);
                    case '|':
                        return (T_BAR);
                    case '<':
                        return (T_LT);
                    case '\n':
                        return (T_NL);
                    case ' ':
                    case '\t':
                        continue;
                    case '>':
                        state = GTGT;
                        continue;
                    case '"':
                        state = INQUOTE;
                        continue;
                    default:
                        state = INWORD;
                        *w++ = c;
                        continue;
                }
            case GTGT:
                if(c == '>') return (T_GTGT);
                un_getc(c);
                return (T_GT);
            case INQUOTE:
                switch(c)
                {
                    case '\\':
                        *w++ = get_char();
                        continue;
                    case '"':
                        *w = '\0';
                        return (T_WORD);
                    default:
                        *w++ = c;
                        continue;
                }
            case INWORD:
                switch(c)
                {
                    case ';':
                    case '&':
                    case '|':
                    case '<':
                    case '>':
                    case '\n':
                    case ' ':
                    case '\t':
                        un_getc(c);
                        *w = '\0';
                        return (T_WORD);
                    default:
                        *w++ = c;
                        continue;
                }
        }
    }
    return (T_EOF);
}

/* do simple command */
TOKEN command(int *waitpid, BOOLEAN makepipe, int *pipefd)
{
    TOKEN token, term;
    int argc, srcfd, dstfd, pid, pfd[ 2 ];
    char *argv[ MAX_ARG + 1 ], srcfile[ MAX_FNAME ], dstfile[ MAX_FNAME ];
    char word[ MAX_WORD ], subst[ MAX_WORD ];
    BOOLEAN append;

    argc = srcfd = 0;
    dstfd = 1;
    while(1)
    {
        switch(token = gettoken(word))
        {
            case T_WORD:
                if(argc == MAX_ARG)
                {
                    fprintf(stderr, "Too many args\n");
                    break;
                }
                if(substitute(word, subst, MAX_WORD) != 0)
                {
                    // the parse error message is handled in substitute
                    break;
                }
                if((argv[ argc ] = (char *) malloc(strlen(subst) + 1)) == NULL)
                {
                    fprintf(stderr, "Out of argument memory\n");
                    break;
                }
                strcpy(argv[ argc ], subst);
                argc++;
                continue;
            case T_LT:
                if(makepipe)
                {
                    fprintf(stderr, "Extra <\n");
                    break;
                }
                if(gettoken(subst) != T_WORD)
                {
                    fprintf(stderr, "Illegal <\n");
                    break;
                }
                if(substitute(subst, srcfile, MAX_FNAME) != 0)
                {
                    // error message handled in substitute
                    break;
                }
                srcfd = BADFD;
                continue;
            case T_GT:
            case T_GTGT:
                if(dstfd != 1)
                {
                    fprintf(stderr, "Extra > or >>\n");
                    break;
                }
                if(gettoken(subst) != T_WORD)
                {
                    fprintf(stderr, "Illegal > or >>\n");
                    break;
                }
                if(substitute(subst, dstfile, MAX_FNAME) != 0)
                {
                    // error message handled in substitute
                    break;
                }
                dstfd = BADFD;
                append = token == T_GTGT;
                continue;
            case T_BAR:
            case T_AMP:
            case T_SEMI:
            case T_NL:
                argv[ argc ] = NULL;
                if(token != T_BAR) term = token;
                else
                {
                    if(dstfd != 1)
                    {
                        fprintf(stderr, "> or >> conflicts with |\n");
                        break;
                    }
                    term = command(waitpid, TRUE, &dstfd);
                }
                if(makepipe)
                {
                    if(pipe(pfd) == -1) syserr("in command(), " __FILE__);
                    *pipefd = pfd[ 1 ];
                    srcfd = pfd[ 0 ];
                }
                pid = invoke(argc, argv, srcfd, srcfile, dstfd, dstfile, append, term == T_AMP);
                if(token != T_BAR) *waitpid = pid;
                if(argc == 0 && (token != T_NL || srcfd > 1)) fprintf(stderr, "Missing command\n");
                while(--argc >= 0)
                    free(argv[ argc ]);
                return (term);
            case T_EOF:
                exit(0);
        }
    }
}
