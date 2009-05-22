
#include <dlfcn.h>
#include "codegen.h"

/* Dynamically load libc */
void *_fiu_libc;

/* Recursion counter, per-thread */
int __thread _fiu_called;

static int __attribute__((constructor)) init(void)
{
	_fiu_called = 0;

	_fiu_libc = dlopen("libc.so.6", RTLD_NOW);
	if (_fiu_libc == NULL) {
		printd("Error loading libc: %s\n", dlerror());
		return 0;
	}

	printd("done\n");
	return 1;
}

