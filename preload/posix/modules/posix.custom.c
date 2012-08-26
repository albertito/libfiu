
/*
 * Custom-made wrappers for some special POSIX functions.
 */

#include "codegen.h"


#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>


/* Wrapper for open(), we can't generate it because it has a variable number
 * of arguments */
static int (*_fiu_orig_open) (const char *pathname, int flags, ...) = NULL;

static int _fiu_in_init_open = 0;

static void constructor_attr(201) _fiu_init_open(void)
{
	rec_inc();
	_fiu_in_init_open++;

	_fiu_orig_open = (int (*) (const char *, int, ...))
		libc_symbol("open");

	_fiu_in_init_open--;
	rec_dec();
}

int open(const char *pathname, int flags, ...)
{
	int r;
	int fstatus;

	/* Differences from the generated code begin here */

	mode_t mode;
	va_list l;

	if (flags & O_CREAT) {
		va_start(l, flags);

		/* va_arg() can only take fully promoted types, and mode_t
		 * sometimes is smaller than an int, so we should always pass
		 * int to it, and not mode_t. Not doing so would may result in
		 * a compile-time warning and run-time error. We asume that it
		 * is never bigger than an int, which holds in practise. */
		mode = va_arg(l, int);

		va_end(l);
	} else {
		/* set it to 0, it's ignored anyway */
		mode = 0;
	}

	/* Differences from the generated code end here */

	if (_fiu_called) {
		if (_fiu_orig_open == NULL) {
			if (_fiu_in_init_open) {
				printd("fail on init\n");
				return -1;
			} else {
				printd("get orig\n");
				_fiu_init_open();
			}
		}

		printd("orig\n");
		return (*_fiu_orig_open) (pathname, flags, mode);
	}

	printd("fiu\n");

	/* fiu_fail() may call anything */
	rec_inc();

	/* Use the normal macros to complete the function, now that we have a
	 * set mode to something */

	static const int valid_errnos[] = {
	  #ifdef EACCESS
		EACCES,
	  #endif
	  #ifdef EFAULT
		EFAULT,
	  #endif
	  #ifdef EFBIG
		EFBIG,
	  #endif
	  #ifdef EOVERFLOW
		EOVERFLOW,
	  #endif
	  #ifdef ELOOP
		ELOOP,
	  #endif
	  #ifdef EMFILE
		EMFILE,
	  #endif
	  #ifdef ENAMETOOLONG
		ENAMETOOLONG,
	  #endif
	  #ifdef ENFILE
		ENFILE,
	  #endif
	  #ifdef ENOENT
		ENOENT,
	  #endif
	  #ifdef ENOMEM
		ENOMEM,
	  #endif
	  #ifdef ENOSPC
		ENOSPC,
	  #endif
	  #ifdef ENOTDIR
		ENOTDIR,
	  #endif
	  #ifdef EROFS
		EROFS
	  #endif
	};
mkwrap_body_errno("posix/io/oc/open", -1)
mkwrap_bottom(open, (pathname, flags, mode))


