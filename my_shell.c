#define create_vars 1
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser.h"
#include "substitute.h"
#include "my_shell.h"
#include "syserr.h"
#include "cd.h"

/**
 * MODIFICATIONS:
 *
 *  - add builtin commands cd and exit
 *
 *  - implement the invoke method
 *
 *  - add support for redirections with built-in commands
 *      Notes:
 *      Only the changes to the destfd are supported, since modifying the
 *      stdin for a builtin command does not make sense (echo ".." > cd ??
 *      does not work, at least in zsh !)
 *      I decided to reuse the redirect method. The only thing to do was cutting
 *      the last line (close_all_files) from redirect and pasting it in the invoke
 *      method.
 *
 *  - fix bug when assigning variables:
 *      before, only "export a" was allowed. Defining + exporting at once
 *      (export a=toto) resulted in a variable named a=toto with an empty
 *      value (a=toto=)...
 *      Now, assign + export supported !
 *
 *  - fix bug in asg:
 *
 *      > =hello
 *      [3] 21601 segmentation fault (core dumped)  ./my_shell
 *
 *    To avoid that, check for NULL pointer before calling EVset!
 *
 *  - properly free the environ when an exec fails (cleaner).
 *
 * TODO
 * ====
 *  - make the ctrl+c signal not exit the shell, but kill the current subprocess instead.
 *  - add support for unset varname
 *      cat file.txt | sed 's/^.*$/#/'
 *    will not work, since $ must be escaped. Use instead:
 *      cat file.txt | sed 's/^.*\$/#/'
 *
 * NOTES
 * =====
 *
 * - some commands may have a modified syntax. For example:
 * - it would be nice to have a "help" method, with infos about
 *   variables and such.
 */

/* a real shell */
int main(int argc, char *argv[ ])
{
    char *prompt;
    int pid;
    TOKEN term;

    if(!EVinit()) fatal("fatal: can't initialise environment. Too much environment variables?" __FILE__);
    if((prompt = EVget("PS2")) == NULL) prompt = "~> ";
    printf("Welcome! Type 'tips' to display a list of quick tips, 'exit' to quit the shell.\n");
    printf("%s", prompt);

    while(1)
    {
        term = command(&pid, FALSE, NULL);
        if(term != T_AMP && pid != 0) waitfor(pid);
        if(term == T_NL) printf("%s", prompt);
        close_all_files(3);
    }
    return 0;
}

/* close all file descriptors from start, ignoring errors */
void close_all_files(int sfd)
{
    int fd;

    for(fd = sfd; fd <= FOPEN_MAX; fd++)
        close(fd);
}

/* assignment command */
void asg(int argc, char *argv[ ], BOOLEAN export)
{
    char *name, *val;

    if(argc != 1) printf("error: extra arguments.\n");
    else
    {
        name = strtok(argv[ 0 ], "=");
        val = strtok( NULL, "\1"); /* get all that's left */
        if(name == NULL || val == NULL)
        {
            fprintf(stderr, "assignment error: unexpected format [%s]. Usage: varname=value.\n", argv[0]);
        }
        else if(EVset(name, val))
        {
            if(export) EVexport(name);
        }
        else
        {
            fprintf(stderr, "error: can't assign variable [%s].\n", name);
        }
    }
}

/* environment variable export command */
void vexport(int argc, char *argv[ ])
{
    int i;

    if(argc == 1 && !strchr(argv[ 0 ], '='))
    {
        set(argc, argv);
        return;
    }
    for(i = 1; i < argc; i++)
    {
        if(strchr(argv[ i ], '='))
        {
            // assign and export
            asg(1, &argv[ i ], TRUE);
        }
        else if(!EVexport(argv[ i ]))
        {
            printf("Can't export %s\n", argv[ i ]);
            return;
        }
    }

}

/* set command */
void set(int argc, char *argv[ ])
{
    if(argc != 1) printf("error: extra arguments.\n");
    else EVprint();
}

// -----------------------------------------

/* invoke a simple command */
int invoke(int argc, char *argv[ ], int srcfd, char * srcfile, int dstfd, char * dstfile,
        BOOLEAN append, BOOLEAN bckgrnd)
{
    if(argc == 0 || builtin(argc, argv, srcfd, srcfile, dstfd, dstfile, append, bckgrnd)) return 0;

    pid_t pid = fork();

    if(pid < 0)
    {
        /* error */
        fprintf( stderr, "Error in invoke: fork failed.\n");
        return -1;
    }
    else if(pid == 0)
    {
        /* child */

        // export var
        EVupdate();

        // do redirect
        redirect(srcfd, srcfile, dstfd, dstfile, append, bckgrnd);
        close_all_files(3);

        // launch process
        execvp(argv[ 0 ], argv);

        fprintf(stderr, "Unknown command: [%s].\n", argv[ 0 ]);
        EVfree();   // properly clean up
        exit(EXIT_FAILURE);
    }
    else
    {
        /* parent */
        if(srcfd > 2) close(srcfd);
        if(dstfd > 2) close(dstfd);

        return pid;
    }

}

/* I/O redirection */
void redirect(int srcfd, char *srcfile, int dstfd, char * dstfile, BOOLEAN append, BOOLEAN bckgrnd)
{
    int flags;

    if(srcfd == 0 && bckgrnd)
    {
        strcpy(srcfile, "/dev/null");
        srcfd = BADFD;
    }
    if(srcfd != 0)
    {
        // since we close stdin, the next fd will necessary be placed at index 0 !
        if(close(0) == -1) syserr("error: closing srcfd in redirect()" __FILE__);
        if(srcfd > 0)
        {
            if(dup(srcfd) != 0) fatal("fatal error: can't dup srcfd in redirect()" __FILE__);
        }
        else if(open(srcfile, O_RDONLY, 0) == -1)
        {
            fprintf( stderr, "can't open %s\n", srcfile);
            exit(0);
        }
    }
    if(dstfd != 1)
    {
        // since we close stdout, the next fd will necessary be placed at index 1 !
        if(close(1) == -1) syserr("error: closing dstfd in redirect(), " __FILE__);
        if(dstfd > 1)
        {
            if(dup(dstfd) != 1) fatal("fatal error: can't dup dstfd in redirect(), " __FILE__);
        }
        else
        {
            flags = O_WRONLY | O_CREAT;
            if(!append) flags |= O_TRUNC;
            if(open(dstfile, flags, 0666) == -1)
            {
                fprintf( stderr, "can't create %s\n", dstfile);
                exit(0);
            }
            if(append) if(lseek(1, 0L, 2) == -1) syserr("can't seek in redirect(), " __FILE__);
        }
    }
}

/* wait for child process */
void waitfor(int pid)
{
    int wpid, status;

    while((wpid = wait(&status)) != pid && wpid != -1)
        statusprt(wpid, status);

    if(wpid == pid) statusprt(0, status);
}

// ------- BUILTINS

/*
 * available builtin commands
 */
typedef enum
{
    BUILTIN_NONE,
    BUILTIN_ASSIGN,
    BUILTIN_CD,
    BUILTIN_EXPORT,
    BUILTIN_SET,
    BUILTIN_HISTORY,
    BUILTIN_QUICKTIPS,
    BUILTIN_EXIT
} E_builtin_cmd;

void print_quicktips()
{
    printf("Some quick tips:\n");
    printf(" - use double-quotes to delimit arguments with spaces,\n");
    printf(" - everything prefixed with $ is considered a variable and will be expanded.\n");
    printf(" - an undefined variable is expanded with ''.\n");
    printf(" - escape the $ with \\ to avoid variable expansion.\n");
    printf(" - enter ctrl+D to stop a subprocess waiting for input.\n");
    printf(" - builtins: set, export, [varname]=[value], cd, history, h[1-10], tips, exit\n");
}
/*
 * this function allows us to check if the command is a built-in one before actually
 * making the builtin call. It simply returns the type of command to execute.
 * It is useful for the redirections (use DRY principle, versus doing a redirect in each
 * switch case...)
 */
E_builtin_cmd is_builtin(char * cmd)
{
    if(strchr(cmd, '=') != NULL)    return BUILTIN_ASSIGN;
    if(strcmp(cmd, "export") == 0)  return BUILTIN_EXPORT;
    if(strcmp(cmd, "set") == 0)     return BUILTIN_SET;
    if(strcmp(cmd, "cd") == 0)      return BUILTIN_CD;
    if(strcmp(cmd, "history") == 0) return BUILTIN_HISTORY;
    if(strcmp(cmd, "tips") == 0)    return BUILTIN_QUICKTIPS;
    if(strcmp(cmd, "exit") == 0)    return BUILTIN_EXIT;
    return BUILTIN_NONE;
}

/* execute a built-in command */
BOOLEAN builtin(int argc, char *argv[ ], int srcfd, char *srcfile, int dstfd, char * dstfile,
        BOOLEAN append, BOOLEAN bckgrnd)
{
    E_builtin_cmd cmd = is_builtin(argv[ 0 ]);

    if(cmd == BUILTIN_NONE) return FALSE;   // not a builtin
    if(cmd == BUILTIN_EXIT)
    {
        printf("See you soon!\n");
        exit(0);
    }

    // setup redirect
    int stdout_bk = -1;
    BOOLEAN is_redirect = dstfd != STDOUT_FILENO;

    if(is_redirect)
    {
        stdout_bk = dup(STDOUT_FILENO);
        redirect(srcfd, srcfile, dstfd, dstfile, append, bckgrnd);
    }

    // execute command
    switch(cmd)
    {
        case BUILTIN_ASSIGN:
            asg(argc, argv, FALSE);
            break;

        case BUILTIN_EXPORT:
            vexport(argc, argv);
            break;

        case BUILTIN_CD:
            cd(argc, argv);
            break;

        case BUILTIN_SET:
            set(argc, argv);
            break;

        case BUILTIN_HISTORY:
            print_history();
            break;

        case BUILTIN_QUICKTIPS:
            print_quicktips();
            break;

        default:   // should never happen, but avoids compiler warnings
            break;
    }

    // undo redirect
    if(is_redirect)
    {
        fflush( stdout);

        if(stdout_bk > 0)   // we are never too careful
        {
            dup2(stdout_bk, STDOUT_FILENO);
            close(stdout_bk);
        }
        if(dstfd > STDOUT_FILENO) close(dstfd);
    }

    return TRUE;
}
