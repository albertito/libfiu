
include: <string.h>
include: <errno.h>
include: <stdlib.h>

fiu name base: libc/str/

# glibc, when _GNU_SOURCE is defined, can have macros for strdup/strndup, so
# we need to avoid those.

v: #ifndef strdup
char *strdup(const char *s);
	on error: NULL
	valid errnos: ENOMEM
v: #endif

v: #ifndef strndup
char *strndup(const char *s, size_t n);
	on error: NULL
	valid errnos: ENOMEM
v: #endif

