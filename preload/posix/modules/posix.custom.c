
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

static void constructor_attr(201) _fiu_init_open(void)
{
	rec_inc();
	_fiu_orig_open = (int (*) (const char *, int, ...))
			 dlsym(_fiu_libc, "open");
	rec_dec();
}

int open(const char *pathname, int flags, ...)
{
	int r;
	int fstatus;

	/* Differences from the generated code begin here */

	int mode;
	va_list l;

	if (flags & O_CREAT) {
		va_start(l, flags);
		mode = va_arg(l, mode_t);
		va_end(l);
	} else {
		/* set it to 0, it's ignored anyway */
		mode = 0;
	}

	if (_fiu_called) {
		printd("orig\n");
		return (*_fiu_orig_open) (pathname, flags, mode);
	}

	/* Differences from the generated code end here */

	printd("fiu\n");

	/* fiu_fail() may call anything */
	rec_inc();

	/* Use the normal macros to complete the function, now that we have a
	 * set mode to something */

	int valid_errnos[] = { EACCES, EFAULT, EFBIG, EOVERFLOW, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENOENT, ENOMEM, ENOSPC, ENOTDIR, EROFS };
mkwrap_body_errno("posix/io/oc/open", -1, 13)
mkwrap_bottom(open, (pathname, flags, mode))


