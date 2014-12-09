/**
 * @file cd.c
 * 
 * @date 2 Dec 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "environ.h"

#define OLDPWD  "OLDPWD"
#define PWD     "PWD"

void cd( int argc, char * argv[ ] )
{
    if( argc != 2 )
    {
        fprintf( stderr, "usage: cd [dir]\n" );
        return;
    }

    char * arg = argv[1];
    char * pwd, * npwd;

    char buff[1024];

    pwd = EVget(PWD);

    if(strcmp(arg, "-") == 0)
    {
        npwd = EVget(OLDPWD);
    }
    else if(arg[0] == '~')
    {
        char * home = EVget("HOME");
        strncpy(buff, home, 1024);
        strncpy(buff + strlen(home), arg+1, 1024);
        npwd = buff;
    }
    else
    {
        npwd = arg;
    }


    if(chdir(npwd) == 0)
    {
        int ok = EVset(OLDPWD, pwd);
        // get absolute path
        npwd = get_current_dir_name();
        ok &= EVset(PWD, npwd);
        free(npwd);

        if(ok) printf("CWD: %s\n", EVget(PWD));
    }
    else
    {
        fprintf(stderr, "Could not cwd, %s does not exist.\n", npwd);
    }

    return;
}

