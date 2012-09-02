
/* Some libfiu's internal declarations */

#ifndef _INTERNAL_H
#define _INTERNAL_H

/* Recursion count, used both in fiu.c and fiu-rc.c */
extern __thread int rec_count;

/* Gets a stack trace. The pointers are stored in the given buffer, which must
 * be of the given size. The number of entries is returned.
 * It's a wrapper around glibc's backtrace(). */
int get_backtrace(void *buffer, int size);

/* Returns a pointer to the start of the function containing the given code
 * address, or NULL if it can't find any. */
void *get_func_end(void *pc);

/* Returns a pointer to the end of the given function. */
void *get_func_start(void *func);

/* Returns a pointer to the function given by name. */
void *get_func_addr(const char *func_name);

/* Do the above backtrace-related functions work?
 * Takes a pointer to the caller so it can verify it's on the stack. */
int backtrace_works(void (*caller)());

#endif

