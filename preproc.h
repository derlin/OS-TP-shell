/**
 * @file history.h
 * 
 * @date 27 Nov 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */

#ifndef HISTORY_H_
#define HISTORY_H_

/* history management */
void print_history();
void add_history( char * cmd );
char * get_history_at();

/* preprocessing */
char get_char();
void un_getc(char c);

#endif /* HISTORY_H_ */
