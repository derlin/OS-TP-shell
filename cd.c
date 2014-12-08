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
    char * pwd, *oldpwd, * npwd;
    char buff[1024];

    pwd = EVget(PWD);
    oldpwd = EVget(OLDPWD);


    if(strcmp(arg, "-") == 0)
    {
        strcpy(buff, oldpwd);
        npwd = buff;
    }
    else if(arg[0] == '~')
    {
        char * home = EVget("HOME");
        strcpy(buff, home);
        strcpy(buff + strlen(home), arg+1);
        npwd = buff;
    }
    else if(arg[0] == '/')
    {
        npwd = arg;
    }
    else
    {
        char relpath[1024];
        sprintf(relpath, "%s/%s", pwd, arg);
        realpath(relpath, buff);
        npwd = buff;
    }


//    struct stat s;set
//    stat(npwd, &s);
//    if(S_ISDIR(s.st_mode))
//    {
    if(chdir(npwd) == 0)
    {
        int ok = EVset(OLDPWD, pwd);
        ok &= EVset(PWD, npwd);
        if(ok) printf("CWD: %s\n", EVget(PWD));
    }
    else
    {
        fprintf(stderr, "Could not cwd, %s does not exist.\n", npwd);
    }

    return;
}

