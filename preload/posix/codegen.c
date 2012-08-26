
#include <stdio.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <stdlib.h>
#include "codegen.h"
#include "build-env.h"

/* Recursion counter, per-thread */
int __thread _fiu_called = 0;

/* Let the user know if there is no constructor priorities support, just in
 * case there are bugs when building/running without them */
#ifdef NO_CONSTRUCTOR_PRIORITIES
#warning "Building without using constructor priorities"
#endif

/* Get a symbol from libc.
 * This function is a wrapper around dlsym(libc, ...), that we use to abstract
 * away how we get the libc wrapper, because on some platforms there are
 * better shortcuts. */
void *libc_symbol(const char *symbol)
{
#ifdef RTLD_NEXT
	return dlsym(RTLD_NEXT, symbol);
#else
	/* We don't want to get this over and over again, so we set it once
	 * and reuse it afterwards. */
	static void *_fiu_libc = NULL;

	if (_fiu_libc == NULL) {
		_fiu_libc = dlopen(LIBC_SONAME, RTLD_NOW);
		if (_fiu_libc == NULL) {
			fprintf(stderr, "Error loading libc: %s\n", dlerror());
			exit(1);
		}
	}

	return dlsym(_fiu_libc, symbol);
#endif
}


/* this runs after all function-specific constructors */
static void constructor_attr(250) _fiu_init_final(void)
{
	struct timeval tv;

	rec_inc();

	fiu_init(0);

	/* since we use random() in the wrappers, we need to seed it */
	gettimeofday(&tv, NULL);
	srandom(tv.tv_usec);

	rec_dec();
}

