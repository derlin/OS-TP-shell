/**
 * @file substitute.h
 * 
 * @date 13 Dec 2014
 * @author Lucy Linder <lucy.derlin@gmail.com>
 *
 */

#ifndef SUBSTITUTE_H_
#define SUBSTITUTE_H_


/**
 * Substitute variables in the given string.
 * @param src the string to parse for variables
 * @return a pointer to a string after variable expansion,
 * or null if no variable was present in src.
 */
char * substitute(char * src);

/**
 * Utility function: expand all the arguments.
 * @param argc the number of arguments
 * @param argv the arguments
 */
void substitute_args(int argc, char * argv[]);

#endif /* SUBSTITUTE_H_ */
