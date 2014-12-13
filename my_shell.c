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
 *      I could not find a way to follow the DRY principle...
 *      So I used pre-run and post-run routines, which could also have
 *      been implemented in a macro.
 *      Only the changes to the destfd are supported, since modifying the
 *      stdin for a builtin command does not make sense (echo ".." > cd ??
 *      does not work, at least in zsh !)
 *      I decided to reuse the redirect method. The only thing to do was cuting
 *      the last line (close_all_files) from redirect and pasting it in the invoke
 *      methodd.
 *
 *  - fix bug when assigning variables:
 *      before, only "export a" was allowed. Defining + exporting at once
 *      (export a=toto) resulted in a variable named a=toto with an empty
 *      value...
 *      Now, assign + export supported !
 *
 *
 *
 * TODO
 * ====
 *
 *  - currently, typing
 *      > grep B
 *    would block the shell. I would be nice to catch the ctrl+c sognal and not exiting the shell,
 *    but the current subprocess instead.
 *
 */


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
            if( export ) EVexport( name );
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
        if( strchr( argv[ i ], '=' ) )
        {
            // assign and export
            asg( 1, &argv[ i ], TRUE );
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
    int i = 0; for(; i < argc; i++) substitute(argv[i], argv[i]);
    if( argc == 0 || builtin( argc, argv, srcfd, srcfile, dstfd, dstfile, append, bckgrnd ) )
        return 0;

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
        redirect( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
        close_all_files( 3 );
        execvp( argv[ 0 ], argv );

        fprintf( stderr, "exec failed: %s\n", argv[0] );
        exit( EXIT_FAILURE );
    }
    else
    {
        /* parent */
        if( srcfd > 2 ) close( srcfd );
        if( dstfd > 2 ) close( dstfd );

        return pid;
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
        // since we close stdin, the next fd will necessary be placed at index 0 !
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
        // since we close stdout, the next fd will necessary be placed at index 1 !
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
}

/* wait for child process */
void waitfor( int pid )
{
    int wpid, status;

    while( ( wpid = wait( &status ) ) != pid && wpid != -1 )
        statusprt( wpid, status );

    if( wpid == pid ) statusprt( 0, status );
}

// -------

static int stdout_bk = -1; // backup of the previous stdout fileno (builtin redirect)

/* backup the current STDOUT_FILENO fd and do the redirect */
void builtin_redirect_begin( int srcfd, char *srcfile, int dstfd, char * dstfile, BOOLEAN append,
        BOOLEAN bckgrnd )
{
    stdout_bk = -1;

    if( dstfd != STDOUT_FILENO )
    {
        stdout_bk = dup( STDOUT_FILENO );
        redirect( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
    }
}

/* set the STDOUT_FILENO fd to its old state and close all files */
void builtin_redirect_end( int srcfd, char *srcfile, int dstfd, char * dstfile, BOOLEAN append,
        BOOLEAN bckgrnd )
{
    if( dstfd != STDOUT_FILENO )
    {
        fflush( stdout );

        if( stdout_bk > 0 )
        {
            dup2( stdout_bk, STDOUT_FILENO );
            close( stdout_bk );
        }
        if( dstfd > STDOUT_FILENO ) close( dstfd );
    }
}

// -------

/* do a built-in command */
BOOLEAN builtin( int argc, char *argv[ ], int srcfd, char *srcfile, int dstfd, char * dstfile,
        BOOLEAN append, BOOLEAN bckgrnd )
{

    if( strchr( argv[ 0 ], '=' ) != NULL ) // assignment
    {
        builtin_redirect_begin( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
        asg( argc, argv, FALSE );
        builtin_redirect_end( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
    }
    else if( strcmp( argv[ 0 ], "export" ) == 0 ) // export name [,names] | export name=value
    {
        builtin_redirect_begin( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
        vexport( argc, argv );
        builtin_redirect_end( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
    }
    else if( strcmp( argv[ 0 ], "set" ) == 0 ) // print the environment variables
    {
        builtin_redirect_begin( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
        set( argc, argv );
        builtin_redirect_end( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
    }
    else if( strcmp( argv[ 0 ], "history" ) == 0 ) // display the last 10 commands
    {
        builtin_redirect_begin( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
        print_history();
        builtin_redirect_end( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
    }
    else if( strcmp( argv[ 0 ], "cd" ) == 0 ) // change directory
    {
        builtin_redirect_begin( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
        cd( argc, argv );
        builtin_redirect_end( srcfd, srcfile, dstfd, dstfile, append, bckgrnd );
    }
    else if( strcmp( argv[ 0 ], "exit" ) == 0 ) exit( 0 ); // exit
    else return ( FALSE );
    return ( TRUE );
}

