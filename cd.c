/**
 * @file cd.c
 * 
 * @date 2 Dec 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 * PURPOSE:
 * this file implements the cd function. It supports absolute and relative
 * paths, as well as cd - (back to previous directory) and the ~ (shortcut for
 * user home directory).
 *
 * NOTES:
 * i decided to return an error when cd is called without arguments. Another
 * possibility would have been to use the default value "user_home" (as in zsh).
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "environ.h"

#define OLDPWD      "OLDPWD"
#define PWD         "PWD"
#define HOME_DIR    "HOME"

/**
 * recode the get_current_dir_name from unistd, since the latter
 * gives compile errors (implicit declaration (??) )
 * @return the current working directory, as a [mallocated] string
 */
char * current_dir_name()
{
    char buff[ 1024 ], *cwd = NULL;
    if(getcwd(buff, sizeof(buff)) != NULL &&   //
            (cwd = (char*) malloc(strlen(buff) + 1)) != NULL)
    {
        strcpy(cwd, buff);
    }
    else
    {
        fprintf(stderr, "cd: could not retrieve cwd.\n");
        exit(1);
    }

    return cwd;
}

void update_cwd(char * npwd)
{
    if(chdir(npwd) == 0)   // change dir ok
    {
        // update environment variables
        char * pwd;
        pwd = EVget( PWD);
        int ok = EVset( OLDPWD, pwd);

        npwd = current_dir_name();   // get absolute path
        ok &= EVset( PWD, npwd);
        free(npwd);

        if(ok) printf("  CWD: %s\n", EVget( PWD));   // feedback
    }
    else   // change dir failed
    {
        fprintf( stderr, "cd: no such file or directory '%s'\n", npwd);
    }
}

// --------------------------------------------------

void cd(int argc, char * argv[ ])
{
    if(argc != 2)   // not exactly one argument, print usage
    {
        fprintf( stderr, "usage: cd [dir]\n");
        return;
    }

    char * arg = argv[ 1 ];

    if(strcmp(arg, "-") == 0)   // back to OLDPWD
    {
        update_cwd(EVget(OLDPWD));
    }
    else if(arg[ 0 ] == '~')   // replace ~ with user_home
    {
        char * home = EVget( HOME_DIR);
        char buff[ 1024 ];

        strncpy(buff, home, 1024);
        strncpy(buff + strlen(home), arg + 1, 1024);

        update_cwd(buff);
    }
    else   // relative or absolute path, no special treatment
    {
        update_cwd(arg);
    }
}

