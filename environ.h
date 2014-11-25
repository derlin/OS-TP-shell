#ifndef ENVIRON_H_
#define ENVIRON_H_

   #include "defs.h"

   BOOLEAN EVset(char *name, char *val);    /* add name & value to environment */
   BOOLEAN EVexport(char *name);            /* set variable to be exported */
   char *  EVget(char *name);               /* get value of variable */
   BOOLEAN EVinit();                        /* initialize symbol table from environment */
   BOOLEAN EVupdate();                      /* build environment from symbol table */
   void    EVprint();                       /* print environment */

#endif
