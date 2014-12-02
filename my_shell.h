#ifndef MY_SHELL_H_
#define MY_SHELL_H_

   #include <errno.h>
   #include <fcntl.h>
   #include "defs.h"
   #include "environ.h"

   void asg(int argc, char *argv[], BOOLEAN);     /* assignment command */
   void vexport(int argc, char *argv[]); /* export command */
   void set(int argc, char *argv[]);     /* set commnand */

   void close_all_files(int start);      /* close all fds, ignore errors */
   int invoke(int argc, char *argv[], int srcfd, char * srcfile, 
              int dstfd, char *dstfile, BOOLEAN append, BOOLEAN bckgrnd);  /* invoke a simple command */
   void redirect(int srcfd, char *srcfile, int dstfd, char * dstfile,      /* I/O redirection */
                 BOOLEAN append, BOOLEAN bckgrnd); 
   void waitfor(int pid);                                      /* wait for child process */

   BOOLEAN builtin(int argc, char *argv[], int srcfd, int dstfd);  /* do a built-in command */

#endif
