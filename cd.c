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

void cd( int argc, char * argv[ ] )
{
    if( argc != 2 ) // not exactly one argument, print usage
    {
        fprintf( stderr, "usage: cd [dir]\n" );
        return;
    }

    char * arg = argv[1];
    char * pwd, * npwd; // current and new cwd

    char buff[1024]; // to append paths in case of "~"

    pwd = EVget(PWD); // get current cwd

    if(strcmp(arg, "-") == 0) // back to OLDPWD
    {
        npwd = EVget(OLDPWD);
    }
    else if(arg[0] == '~') // replace ~ with user_home
    {
        char * home = EVget(HOME_DIR);
        strncpy(buff, home, 1024);
        strncpy(buff + strlen(home), arg+1, 1024);
        npwd = buff;
    }
    else // relative or absolute path, do nothing
    {
        npwd = arg;
    }


    if(chdir(npwd) == 0) // change dir ok
    {
        // update environment variables
        int ok = EVset(OLDPWD, pwd);
        npwd = get_current_dir_name(); // get absolute path
        ok &= EVset(PWD, npwd);
        free(npwd);

        if(ok) printf("CWD: %s\n", EVget(PWD)); // feedback
    }
    else // change dir failed
    {
        fprintf(stderr, "Could not cwd, %s does not exist.\n", npwd);
    }

    return;
}

