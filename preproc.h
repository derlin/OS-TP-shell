/**
 * @file history.h
 * 
 * @date 27 Nov 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */

#ifndef HISTORY_H_
#define HISTORY_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



void print_history();
void add_history( char * cmd );
char * get_history_at();

char get_char();
void un_getc(char c);


void substitute(char * src, char * destination);

#endif /* HISTORY_H_ */
