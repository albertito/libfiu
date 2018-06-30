/* Dummy file used at build time to find out libc's soname.
 * It must use something from libc. See the Makefile for details. */

#include <stdio.h>

int use_printf(void) {
	return printf("I'm using libc\n");
}
