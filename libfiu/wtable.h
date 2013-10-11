
/* Wildcard table.
 *
 * This is a simple key-value table that associates a key (\0-terminated
 * string) with a value (void *).
 *
 * See wtable.c for more information. */

#ifndef _WTABLE_H
#define _WTABLE_H

#include <stdbool.h>		/* for bool */

typedef struct wtable wtable_t;

wtable_t *wtable_create(void (*destructor)(void *));
void wtable_free(wtable_t *t);

void *wtable_get(wtable_t *t, const char *key);
bool wtable_set(wtable_t *t, const char *key, void *value);
bool wtable_del(wtable_t *t, const char *key);

#endif

