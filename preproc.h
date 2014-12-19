/**
 * @file preproc.h
 * 
 * @date 27 Nov 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */

#ifndef HISTORY_H_
#define HISTORY_H_

#define HISTORY_SIZE   10           /* max number of entries in history */

/* history management */
void print_history();               /* print the index + history commands */
void add_history(char * cmd);       /* add a line to history */
char * get_history_at(int index);   /* get the history at index i (between 1 and 10) */

/* preprocessing */
char get_char();                    /* emulation of getchar() */
void un_getc(char c);               /* emulation of ungetc() */

#endif /* HISTORY_H_ */
