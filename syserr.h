#ifndef SYSERR_H_
#define SYSERR_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

   /* print system call error message and return error number */
   void syserr(char *msg);

   /* print error message and terminate */
   void fatal(char *msg);
   
   /* interpret status code */
   void statusprt(int pid, int status);

#endif

