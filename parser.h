#ifndef PARSER_H_
#define PARSER_H_

   #include <stdio.h>
   #include <string.h>

   #include "environ.h"
   #include "defs.h"
   #include "my_shell.h"

   #ifdef create_vars
      const int BADFD = -2;                /* some non-existing file descriptor */
      const int MAX_ARG=100;               /* maximum arguments for a command */
      const int MAX_FNAME = 256;           /* maximum filename length */
      const int MAX_WORD = 256;            /* maximum command and argument length */
   #else
      extern const int BADFD;
      extern const int MAX_ARG;
      extern const int MAX_FNAME;
      extern const int MAX_WORD;
   #endif

   typedef enum {T_WORD, T_BAR, T_AMP, T_SEMI, T_GT, T_GTGT, T_LT, T_NL, T_EOF} TOKEN;

   TOKEN gettoken(char *word); /* collect and classify token*/
   TOKEN command(int *waitpid, BOOLEAN makepipe, int *pipefd); /* do simple command */
#endif

