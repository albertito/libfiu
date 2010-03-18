
#include <stdio.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <stdlib.h>
#include "codegen.h"
#include "build-env.h"

/* Dynamically load libc */
void *_fiu_libc;

/* Recursion counter, per-thread */
int __thread _fiu_called = 0;

/* Let the user know if there is no constructor priorities support, just in
 * case there are bugs when building/running without them */
#ifdef NO_CONSTRUCTOR_PRIORITIES
#warning "Building without using constructor priorities"
#endif

void constructor_attr(200) _fiu_init(void)
{
	static int initialized = 0;

	/* When built without constructor priorities, we could be called more
	 * than once during the initialization phase: one because we're marked
	 * as a constructor, and another when one of the other constructors
	 * sees that it doesn't have _fiu_libc set. */

	printd("_fiu_init() start (%d)\n", initialized);
	if (initialized)
		goto exit;

	_fiu_libc = dlopen(LIBC_SONAME, RTLD_NOW);
	if (_fiu_libc == NULL) {
		fprintf(stderr, "Error loading libc: %s\n", dlerror());
		exit(1);
	}
	initialized = 1;

exit:
	printd("_fiu_init() done\n");
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

