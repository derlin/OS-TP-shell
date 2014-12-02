#include "environ.h"
#include <string.h>
#include <stdlib.h>

extern char **environ;

/* static const int MAXVAR = 100;      /* max symbol table entries for env vars */
static const int MAXVARNAMELEN = 40;   /* max symbol name length */
#define MAXVAR 100

static struct varslot {          /* symbol table */
   char *name;       /* variable name */
   char *val;        /* var value */
   BOOLEAN exported; /* is this to be exported ? */
} sym[MAXVAR];

/* find symbol table entry */ 
static struct varslot *find(char *name) { 
   int i ; 
   struct varslot *v = NULL; 

   for (i = 0; i <MAXVAR; i++) 
      if (sym[i].name == NULL) {
         if (v == NULL) 
            v = &sym[i]; 
      } 
      else if (strcmp(sym[i].name, name) == 0) { 
         v = &sym[i];
         break;
      } 
   return(v); 
} 

/* initialize name or value */
static BOOLEAN assign(char **p, char *s) {
   int size = strlen(s) + 1;

   if (*p == NULL) {
      if ((*p = (char *) malloc(size)) == NULL)
         return(FALSE);
   }
   else if ((*p = (char *) realloc(*p, size)) == NULL)
      return(FALSE);
   strcpy(*p, s);
   return(TRUE);
}


/* add name & value of environment */
BOOLEAN EVset(char *name, char *val) {
   struct varslot *v;

   if ((v = find(name)) == NULL)
      return(FALSE);
   return (assign(&v->name, name) && assign(&v->val, val));
}


/* set variable to be exported */
BOOLEAN EVexport(char *name) {
   struct varslot *v;

   if ((v = find(name)) == NULL)
      return(FALSE);
   if (v->name == NULL)
      if (! assign(&v->name, name) || ! assign(&v->val, ""))
         return(FALSE);
   v->exported = TRUE;
   return(TRUE);
}


/* get value of variable */
char *EVget(char *name) {
   struct varslot *v;

   if ((v = find(name)) == NULL || v->name == NULL)
      return(NULL);
   return(v->val);
}


/* initialize symbol table from environment */
BOOLEAN EVinit() {
   int i, namelen;
   char name[MAXVARNAMELEN];

   for (i=0; environ[i] != NULL; i++) {
      namelen = strcspn(environ[i], "=");
      strncpy(name, environ[i], namelen);
      name[namelen] = '\0';
      if (!EVset(name, &environ[i][namelen+1]) || !EVexport(name))
         return(FALSE);
   }
   return(TRUE);
}


/* build environment from symbol table */
BOOLEAN EVupdate() {
   int i, envi, nvlen;
   struct varslot *v;
   static BOOLEAN updated = FALSE;

   if (! updated)
      if ((environ = (char **) malloc((MAXVAR + 1) * sizeof(char *))) == NULL)
         return(FALSE);
   envi = 0;
   for (i=0; i < MAXVAR; i++) {
      v = &sym[i];
      if (v->name == NULL || ! v->exported)
         continue;
      nvlen = strlen(v->name) + strlen(v->val) + 2;
      if (!updated) {
         if ((environ[envi] = (char *) malloc(nvlen)) == NULL)
            return(FALSE);
      }
      else if ((environ[envi] = (char *) realloc(environ[envi], nvlen)) == NULL)
          return(FALSE);
      sprintf(environ[envi], "%s=%s", v->name, v->val);
      envi++;
   }
   environ[envi] = NULL;
   updated = TRUE;
   return(TRUE);
}

/* print environment */
void EVprint() {
   int i;

   for (i=0; i < MAXVAR; i++)
      if (sym[i].name != NULL)
         printf("%3s %s=%s\n", sym[i].exported ? "[E]" : "", sym[i].name, sym[i].val);

}

