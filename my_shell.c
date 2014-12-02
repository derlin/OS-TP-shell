#define create_vars 1
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser.h"
#include "preproc.h"
#include "my_shell.h"
#include "syserr.h"

/* a real shell */
int main( int argc, char *argv[ ] )
{
    char *prompt;
    int pid;
    TOKEN term;

    if( !EVinit() ) fatal( "can't initialize environment in main(), " __FILE__ );
    if( ( prompt = EVget( "PS2" ) ) == NULL ) prompt = "fmk-> ";
    printf( "%s", prompt );

    while( 1 )
    {
        term = command( &pid, FALSE, NULL );
        if( term != T_AMP && pid != 0 ) waitfor( pid );
        if( term == T_NL ) printf( "%s", prompt );
        close_all_files( 3 );
    }
    return 0;
}

/* close all file descriptors from start, ignoring errors */
void close_all_files( int sfd )
{
    int fd;

    for( fd = sfd; fd <= FOPEN_MAX; fd++ )
        close( fd );
}

/* assignment command */
void asg( int argc, char *argv[ ], BOOLEAN export )
{
    char *name, *val;

    if( argc != 1 ) printf( "Extra args\n" );
    else
    {
        name = strtok( argv[ 0 ], "=" );
        val = strtok( NULL, "\1" ); /* get all that's left */
        if( EVset( name, val ) )
        {
            if(export) EVexport(name);
        }
        else
        {
            printf( "Can't assign\n" );
        }
    }
}

/* environment variable export command */
void vexport( int argc, char *argv[ ] )
{
    int i;

    if( argc == 1 && !strchr( argv[ 0 ], '=' ) )
    {
        set( argc, argv );
        return;
    }
    for( i = 1; i < argc; i++ )
    {
        if(strchr( argv[ i ], '=' ))
        {
            // assign and export
            asg(1, &argv[i], TRUE);
        }
        else if( !EVexport( argv[ i ] ) )
        {
            printf( "Can't export %s\n", argv[ i ] );
            return;
        }
    }

}

/* set command */
void set( int argc, char *argv[ ] )
{
    if( argc != 1 ) printf( "Extra args\n" );
    else EVprint();
}

/* invoke a simple command */
int invoke( int argc, char *argv[ ], int srcfd, char * srcfile, int dstfd, char * dstfile,
        BOOLEAN append, BOOLEAN bckgrnd )
{
    if( argc == 0 || builtin( argc, argv, srcfd, dstfd ) ) return 0;

    // export var
    EVupdate();

    // TODO : redirections
    pid_t pid = fork();

    if( pid < 0 )
    {
        /* error */
        fprintf( stderr, "fork failed\n" );
        return -1;
    }
    else if( pid == 0 )
    {
        /* child */
        redirect( srcfd, srcfile, dstfd, dstfile, append, bckgrnd);
        execvp( argv[ 0 ], argv );

        fprintf( stderr, "exec failed\n" );
        exit( EXIT_FAILURE );
    }
    else
    {
        // TODO
        /* parent */
        int status = 0;
        if(!bckgrnd){
            // wait for the child to exit
            waitpid(pid, &status, 0);
        }
        return status;
    }

}

/* I/O redirection */
void redirect( int srcfd, char *srcfile, int dstfd, char * dstfile, BOOLEAN append,
        BOOLEAN bckgrnd )
{
    int flags;

    if( srcfd == 0 && bckgrnd )
    {
        strcpy( srcfile, "/dev/null" );
        srcfd = BADFD;
    }
    if( srcfd != 0 )
    {
        if( close( 0 ) == -1 ) syserr( "closing srcfd in redirect()" __FILE__ );
        if( srcfd > 0 )
        {
            if( dup( srcfd ) != 0 ) fatal( "can't dup srcfd in redirect()" __FILE__ );
        }
        else if( open( srcfile, O_RDONLY, 0 ) == -1 )
        {
            fprintf( stderr, "can't open %s\n", srcfile );
            exit( 0 );
        }
    }
    if( dstfd != 1 )
    {
        if( close( 1 ) == -1 ) syserr( "closing dstfd in redirect(), " __FILE__ );
        if( dstfd > 1 )
        {
            if( dup( dstfd ) != 1 ) fatal( "can't dup dstfd in redirect(), " __FILE__ );
        }
        else
        {
            flags = O_WRONLY | O_CREAT;
            if( !append ) flags |= O_TRUNC;
            if( open( dstfile, flags, 0666 ) == -1 )
            {
                fprintf( stderr, "can't create %s\n", dstfile );
                exit( 0 );
            }
            if( append )
                if( lseek( 1, 0L, 2 ) == -1 ) syserr( "can't seek in redirect(), " __FILE__ );
        }
    }
    close_all_files( 3 );
}

/* wait for child process */
void waitfor( int pid )
{
    int wpid, status;

    while( ( wpid = wait( &status ) ) != pid && wpid != -1 )
        statusprt( wpid, status );
    if( wpid == pid ) statusprt( 0, status );
}

/* do a built-in command */
BOOLEAN builtin( int argc, char *argv[ ], int srcfd, int dstfd )
{
    if( strchr( argv[ 0 ], '=' ) != NULL ) asg( argc, argv, FALSE );
    else if( strcmp( argv[ 0 ], "export" ) == 0 ) vexport( argc, argv );
    else if( strcmp( argv[ 0 ], "set" ) == 0 ) set( argc, argv );
    else if( strcmp( argv[ 0 ], "exit" ) == 0 ) exit( 0 );
    else if( strcmp( argv[ 0 ], "history" ) == 0 ) print_history();
//    else if( strcmp( argv[ 0 ], "cd" ) == 0 ) cd(argc, argv);
    else return ( FALSE );
    if( srcfd != 0 || dstfd != 1 ) fprintf( stderr, "illegal redirection or pipeline\n" );
    return ( TRUE );
}


//void cd(int argc, char * argv[])
//{
//    ;
//}

