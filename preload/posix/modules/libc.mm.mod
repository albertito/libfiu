
include: <errno.h>
include: <stdlib.h>

fiu name base: libc/mm/


void *malloc(size_t size);
	on error: NULL
	valid errnos: ENOMEM

void *calloc(size_t nmemb, size_t size);
	on error: NULL
	valid errnos: ENOMEM

void *realloc(void *ptr, size_t size);
	on error: NULL
	valid errnos: ENOMEM

# Note we don't wrap free() as it does not return anything.

