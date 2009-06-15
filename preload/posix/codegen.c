
#include <stdio.h>
#include <dlfcn.h>
#include <sys/time.h>
#include "codegen.h"

/* Dynamically load libc */
void *_fiu_libc;

/* Recursion counter, per-thread */
int __thread _fiu_called;

static void __attribute__((constructor(200))) _fiu_init(void)
{
	_fiu_called = 0;

	_fiu_libc = dlopen("libc.so.6", RTLD_NOW);
	if (_fiu_libc == NULL) {
		printf("Error loading libc: %s\n", dlerror());
		return;
	}

	printd("done\n");
}

/* this runs after all function-specific constructors */
static void __attribute__((constructor(250))) _fiu_init_final(void)
{
	struct timeval tv;

	rec_inc();

	fiu_init(0);

	/* since we use random() in the wrappers, we need to seed it */
	gettimeofday(&tv, NULL);
	srandom(tv.tv_usec);

	rec_dec();
}

