
/*
 * Custom-made wrappers for malloc() and realloc().
 *
 * They can't be made generic because, at least on glibc, they're used before
 * constructors are called.
 *
 * We use __malloc_hook, the glibc-specific interface, so this is glibc-only.
 */

/* We need to include features.h, which in turns defines __GLIBC__. However,
 * features.h is glibc-specific and does not exist on other platforms, so we
 * can't include it directly or the file won't build.
 * Instead, we have to resort to this ugly trick of including limits.h, which
 * is standard and we know glibc's implementation includes features.h.
 */
#include <limits.h>

#ifndef __GLIBC__
  #warning "Not using glibc, so no malloc() wrappers will be available"
#else

#include "codegen.h"

#include <malloc.h>


/* Original glibc's hooks, saved in our init function */
static void *(*old_malloc_hook)(size_t, const void *);
static void *(*old_realloc_hook)(void *ptr, size_t size, const void *caller);

/* Our own hooks, that will replace glibc's */
static void *fiu_malloc_hook(size_t size, const void *caller);
static void *fiu_realloc_hook(void *ptr, size_t size, const void *caller);


/* We are not going to use __malloc_initialize_hook because it would be called
 * before our library initialization functions, which include fiu_init().
 * Instead, we will use a constructor just like the other wrappers, but it
 * will run after them just to make things tidier (NOT because it is
 * necessary). */

static void constructor_attr(202) fiu_init_malloc(void)
{
	/* Save original hooks, used in ours to prevent unwanted recursion */
	old_malloc_hook = __malloc_hook;
	old_realloc_hook = __realloc_hook;

	__malloc_hook = fiu_malloc_hook;
	__realloc_hook = fiu_realloc_hook;
}


static void *fiu_malloc_hook(size_t size, const void *caller)
{
	void *r;
	int fstatus;

	/* fiu_fail() may call anything */
	rec_inc();

	/* See __malloc_hook(3) for details */
	__malloc_hook = old_malloc_hook;

	fstatus = fiu_fail("libc/mm/malloc");
	if (fstatus != 0) {
		r = NULL;
		goto exit;
	}

	printd("calling orig\n");
	r = malloc(size);

exit:
	old_malloc_hook = __malloc_hook;
	__malloc_hook = fiu_malloc_hook;

	rec_dec();
	return r;
}


static void *fiu_realloc_hook(void *ptr, size_t size, const void *caller)
{
	void *r;
	int fstatus;

	/* fiu_fail() may call anything */
	rec_inc();

	/* See __malloc_hook(3) for details */
	__realloc_hook = old_realloc_hook;

	fstatus = fiu_fail("libc/mm/realloc");
	if (fstatus != 0) {
		r = NULL;
		goto exit;
	}

	printd("calling orig\n");
	r = realloc(ptr, size);

exit:
	old_realloc_hook = __realloc_hook;
	__realloc_hook = fiu_realloc_hook;

	rec_dec();
	return r;
}


#endif // defined __GLIBC__

