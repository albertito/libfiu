
include: <string.h>
include: <errno.h>

fiu name base: libc/str/

char *strdup(const char *s);
	on error: NULL
	valid errnos: ENOMEM

char *strndup(const char *s, size_t n);
	on error: NULL
	valid errnos: ENOMEM


