/**
 * test_case.c
 *
 * Toke Høiland-Jørgensen
 * 2016-12-10
 */

#include "stdio.h"

void main(char **argv, int argc) {

	// This is a single line comment
	int a;
	/* This is a normal comment */
	int b;
	/** This is a comment
extending over multiple lines
	*/ int c; // with code at the end of it
	/* And this is a comment // commenting out a comment */ int d;
	printf("Hello, Word"
		" this is a multi-line c string\n" );
	printf("The total number of lines of code should be 8.")
}
