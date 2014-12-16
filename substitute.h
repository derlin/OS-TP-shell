/**
 * @file substitute.h
 * 
 * @date 13 Dec 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */

#ifndef SUBSTITUTE_H_
#define SUBSTITUTE_H_

#define SUBST_OK 0
#define SUBST_OUT_OF_MEM  1
#define SUBST_PARSE_ERROR 2

/**
 * Substitute variables in the given string.
 * @param src the string to parse for variables
 * @param dest where the result will be stored
 * @param len length of dest
 * @return 0 = 0k, 1 = buffer too short, 2 = parse error
 * Note: error messages are printed to stderr
 */

int substitute(char * src, char * dest, int len);

#endif /* SUBSTITUTE_H_ */
