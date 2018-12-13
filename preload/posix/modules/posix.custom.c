
/*
 * Custom-made wrappers for some special POSIX functions.
 */

#include "codegen.h"
#include "hash.h"

#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>


/* Wrapper for open(), we can't generate it because it has a variable number
 * of arguments */
mkwrap_init(int, open, (const char *pathname, int flags, ...),
		(const char *, int, ...))

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
		 * is never bigger than an int, which holds in practice. */
		mode = va_arg(l, int);

		va_end(l);
	} else {
		/* set it to 0, it's ignored anyway */
		mode = 0;
	}

	/* Differences from the generated code end here */

/* Use the normal macros to complete the function, now that we have
 * set mode to something */
mkwrap_body_called(open, (pathname, flags, mode), -1)

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


/* The 64-bit variant for glibc.
 * The code is identical to open(), just using open64() where appropriate. */
#ifdef __GLIBC__

mkwrap_init(int, open64, (const char *pathname, int flags, ...),
		(const char *, int, ...))

int open64(const char *pathname, int flags, ...)
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

/* Use the normal macros to complete the function, now that we have
 * set mode to something */
mkwrap_body_called(open, (pathname, flags, mode), -1)

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
mkwrap_bottom(open64, (pathname, flags, mode))

#endif

/*
 * To simulate ferror() properly, we need to keep track of which FILE * have
 * we returned a failure for.
 *
 * To do that, we keep a set of FILE *. This set is implemented using a hash
 * table, for convenience.
 *
 * An alternative would be to have a static array, but then lookups would
 * scale linearly with the number of files we're expected to fail, and (most
 * importantly) require a new data type just for this.
 */
static hash_t *ferror_hash_table;
static pthread_mutex_t ferror_hash_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t ferror_hash_table_is_initialized = PTHREAD_ONCE_INIT;

static void _fiu_init_ferror_hash_table(void)
{
	rec_inc();
	pthread_mutex_lock(&ferror_hash_table_mutex);

	// Create the hash table. No need for a value destructor, as our values
	// are not pointers but static values.
	ferror_hash_table = hash_create(NULL);

	pthread_mutex_unlock(&ferror_hash_table_mutex);
	rec_dec();
}

/* Our hash table uses character keys, to convert stream to keys we just get
 * the hexadecimal representation via PRIxPTR (%p is implementation
 * dependent). */
#define STREAM_KEY_SIZE (sizeof(uintptr_t) * 2 + 1)

static void stream_to_key(void *stream, char key[STREAM_KEY_SIZE])
{
	snprintf(key, STREAM_KEY_SIZE, "%" PRIxPTR, (uintptr_t) stream);
}

void set_ferror(void * stream)
{
	char key[STREAM_KEY_SIZE];
	stream_to_key(stream, key);

	pthread_once(&ferror_hash_table_is_initialized,
			_fiu_init_ferror_hash_table);

	pthread_mutex_lock(&ferror_hash_table_mutex);

	// Use a dummy the value; we don't care about it, we only need to be able to
	// distinguish it from NULL.
	hash_set(ferror_hash_table, key, (void *) 0xDEAD);

	pthread_mutex_unlock(&ferror_hash_table_mutex);
}

static int get_ferror(void * stream)
{
	char key[STREAM_KEY_SIZE];
	stream_to_key(stream, key);

	pthread_once(&ferror_hash_table_is_initialized,
			_fiu_init_ferror_hash_table);

	pthread_mutex_lock(&ferror_hash_table_mutex);

	void *value = hash_get(ferror_hash_table, key);

	pthread_mutex_unlock(&ferror_hash_table_mutex);

	return value != NULL;
}

static void clear_ferror(void * stream)
{
	char key[STREAM_KEY_SIZE];
	stream_to_key(stream, key);

	pthread_once(&ferror_hash_table_is_initialized,
			_fiu_init_ferror_hash_table);

	pthread_mutex_lock(&ferror_hash_table_mutex);

	hash_del(ferror_hash_table, key);

	pthread_mutex_unlock(&ferror_hash_table_mutex);
}


/* Wrapper for ferror() */
mkwrap_top(int, ferror, (FILE *stream), (stream), (FILE *), 1)
mkwrap_body_hardcoded("posix/stdio/error/ferror", 1)

	/* The following is like mkwrap_bottom(), but has an additional check to
	 * return 1 if we previously returned an error for this file, so the
	 * semantics are consistent. */
	if (_fiu_orig_ferror == NULL)
		_fiu_init_ferror();

	printd("calling orig\n");
	r = (*_fiu_orig_ferror) (stream);

	if (r == 0 && get_ferror(stream)) {
		printd("ferror fixed\n");
		r = 1;
	}

exit:
	rec_dec();
	return r;
}


/* Custom wrapper for clearerr().
 * This function does not fail as such, but we override it so we can provide a
 * consistent view. */
mkwrap_init(void, clearerr, (FILE *stream), (FILE *))

void clearerr (FILE *stream)
{
	rec_inc();

	if (_fiu_orig_clearerr == NULL)
			_fiu_init_clearerr();

	printd("calling orig\n");
	(*_fiu_orig_clearerr) (stream);

	printd("fixing internal state\n");
	clear_ferror(stream);

	rec_dec();
}


/* Custom wrapper for fclose() that clears ferror_hash_table. */
mkwrap_top(int , fclose, (FILE *stream), (stream), (FILE *), (EOF))
	static const int valid_errnos[] = {
	  #ifdef EAGAIN
		EAGAIN,
	  #endif
	  #ifdef EBADF
		EBADF,
	  #endif
	  #ifdef EFBIG
		EFBIG,
	  #endif
	  #ifdef EFBIG
		EFBIG,
	  #endif
	  #ifdef EINTR
		EINTR,
	  #endif
	  #ifdef EIO
		EIO,
	  #endif
	  #ifdef ENOMEM
		ENOMEM,
	  #endif
	  #ifdef ENOSPC
		ENOSPC,
	  #endif
	  #ifdef EPIPE
		EPIPE,
	  #endif
	  #ifdef ENXIO
		ENXIO,
	  #endif
	};
mkwrap_body_errno("posix/stdio/oc/fclose", EOF)

/* We need a custom injection of clear_ferror here to prevent stale entries in
 * ferror_hash_table. */
	clear_ferror(stream);

mkwrap_bottom(fclose, (stream))


/*
 * Variadic functions
 *
 * For some variadic functions, like printf(), we use a non-variadic
 * counterpart (vprintf()) to implement the wrapping.
 *
 */

/* Generate the body for a variadic function, which relies on a non-variadic
 * function for expansion.
 *
 * We expect mkwrap_init to be called on the non-variadic one, see the calls
 * below for examples.
 */
#define mkwrap_variadic_def_and_body_called( \
		RTYPE, /* return type */ \
		NAME, /* variadic name */ \
		NVNAME, /* non-variadic name */ \
		PARAMS, /* parameter definition, with types */ \
		PARAMSN, /* parameter names only, with "arguments" for va part */ \
		LASTPN, /* last parameter name */ \
		ON_ERR /* what to return on error */ \
		) \
								\
	RTYPE NAME PARAMS					\
	{ 							\
		RTYPE r;					\
		int fstatus;					\
		va_list arguments;				\
								\
		if (_fiu_called) {				\
			if (_fiu_orig_##NVNAME == NULL) {	\
				if (_fiu_in_init_##NVNAME) {	\
					printd("fail on init\n"); \
					return ON_ERR;		\
				} else {			\
					printd("get orig\n");	\
					_fiu_init_##NVNAME();	\
				}				\
			}					\
			printd("orig\n");			\
			va_start(arguments, LASTPN);		\
			r = (*_fiu_orig_##NVNAME) PARAMSN;	\
			va_end(arguments);			\
								\
			return r;				\
		}						\
								\
		printd("fiu\n");				\
								\
		/* fiu_fail() may call anything */		\
		rec_inc();


#define mkwrap_variadic_bottom( \
		NAME /* variadic name */, \
		NVNAME /* non-variadic name */, \
		PARAMSN /* parameter names only, with "arguments" for va part */, \
		LASTPN /* last parameter name */ \
		) \
								\
		if (_fiu_orig_##NVNAME == NULL)			\
			_fiu_init_##NVNAME();			\
								\
		printd("calling orig\n");			\
		va_start(arguments, LASTPN);			\
		r = (*_fiu_orig_##NVNAME) PARAMSN;		\
		va_end(arguments);				\
								\
	exit:							\
		rec_dec();					\
		return r;					\
	}


/* fprintf() -> vfprintf() */
mkwrap_init(int, vfprintf,
	(FILE *restrict stream, const char *restrict format, va_list ap),
	(FILE *restrict, const char *restrict, va_list));

mkwrap_variadic_def_and_body_called(int, fprintf, vfprintf,
	(FILE *restrict stream, const char *restrict format, ...),
	(stream, format, arguments),
	format, -1)

	static const int valid_errnos[] = {
	  #ifdef EAGAIN
		EAGAIN,
	  #endif
	  #ifdef EBADF
		EBADF,
	  #endif
	  #ifdef EFBIG
		EFBIG,
	  #endif
	  #ifdef EINTR
		EINTR,
	  #endif
	  #ifdef EIO
		EIO,
	  #endif
	  #ifdef ENOMEM
		ENOMEM,
	  #endif
	  #ifdef ENOSPC
		ENOSPC,
	  #endif
	  #ifdef ENXIO
		ENXIO,
	  #endif
	  #ifdef EPIPE
		EPIPE,
	  #endif
	  #ifdef EILSEQ
		EILSEQ,
	  #endif
	  #ifdef EOVERFLOW
		EOVERFLOW,
	  #endif
	};

mkwrap_body_errno_ferror("posix/stdio/sp/fprintf", -1, stream)
mkwrap_variadic_bottom(fprintf, vfprintf, (stream, format, arguments), format)


/* printf() -> vprintf() */
mkwrap_init(int, vprintf,
	(const char *restrict format, va_list ap),
	(const char *restrict, va_list));

mkwrap_variadic_def_and_body_called(int, printf, vprintf,
	(const char *restrict format, ...),
	(format, arguments),
	format, -1)

	static const int valid_errnos[] = {
	  #ifdef EAGAIN
		EAGAIN,
	  #endif
	  #ifdef EBADF
		EBADF,
	  #endif
	  #ifdef EFBIG
		EFBIG,
	  #endif
	  #ifdef EINTR
		EINTR,
	  #endif
	  #ifdef EIO
		EIO,
	  #endif
	  #ifdef ENOMEM
		ENOMEM,
	  #endif
	  #ifdef ENOSPC
		ENOSPC,
	  #endif
	  #ifdef ENXIO
		ENXIO,
	  #endif
	  #ifdef EPIPE
		EPIPE,
	  #endif
	  #ifdef EILSEQ
		EILSEQ,
	  #endif
	  #ifdef EOVERFLOW
		EOVERFLOW,
	  #endif
	};

mkwrap_body_errno_ferror("posix/stdio/sp/printf", -1, stdout)
mkwrap_variadic_bottom(printf, vprintf, (format, arguments), format)


/* dprintf() -> vdprintf() */
mkwrap_init(int, vdprintf,
	(int fildes, const char *restrict format, va_list ap),
	(int, const char *restrict, va_list));

mkwrap_variadic_def_and_body_called(int, dprintf, vdprintf,
	(int fildes, const char *restrict format, ...),
	(fildes, format, arguments),
	format, -1)

	static const int valid_errnos[] = {
	  #ifdef EAGAIN
		EAGAIN,
	  #endif
	  #ifdef EBADF
		EBADF,
	  #endif
	  #ifdef EFBIG
		EFBIG,
	  #endif
	  #ifdef EINTR
		EINTR,
	  #endif
	  #ifdef EIO
		EIO,
	  #endif
	  #ifdef ENOMEM
		ENOMEM,
	  #endif
	  #ifdef ENOSPC
		ENOSPC,
	  #endif
	  #ifdef ENXIO
		ENXIO,
	  #endif
	  #ifdef EPIPE
		EPIPE,
	  #endif
	  #ifdef EILSEQ
		EILSEQ,
	  #endif
	  #ifdef EOVERFLOW
		EOVERFLOW,
	  #endif
	};

mkwrap_body_errno("posix/stdio/sp/dprintf", -1)
mkwrap_variadic_bottom(dprintf, vdprintf, (fildes, format, arguments), format)


/* fscanf() -> vfscanf() */
mkwrap_init(int, vfscanf,
	(FILE *restrict stream, const char *restrict format, va_list ap),
	(FILE *restrict, const char *restrict, va_list));

mkwrap_variadic_def_and_body_called(int, fscanf, vfscanf,
	(FILE *restrict stream, const char *restrict format, ...),
	(stream, format, arguments),
	format, EOF)

	static const int valid_errnos[] = {
	  #ifdef EAGAIN
		EAGAIN,
	  #endif
	  #ifdef EBADF
		EBADF,
	  #endif
	  #ifdef EINTR
		EINTR,
	  #endif
	  #ifdef EIO
		EIO,
	  #endif
	  #ifdef ENOMEM
		ENOMEM,
	  #endif
	  #ifdef ENXIO
		ENXIO,
	  #endif
	  #ifdef EOVERFLOW
		EOVERFLOW,
	  #endif
	  #ifdef EILSEQ
		EILSEQ,
	  #endif
	  #ifdef EINVAL
		EINVAL,
	  #endif
	};

mkwrap_body_errno_ferror("posix/stdio/sp/fscanf", EOF, stream)
mkwrap_variadic_bottom(fscanf, vfscanf, (stream, format, arguments), format)


/* scanf() -> vscanf() */
mkwrap_init(int, vscanf,
	(const char *restrict format, va_list ap),
	(const char *restrict, va_list));

mkwrap_variadic_def_and_body_called(int, scanf, vscanf,
	(const char *restrict format, ...),
	(format, arguments),
	format, EOF)

	static const int valid_errnos[] = {
	  #ifdef EAGAIN
		EAGAIN,
	  #endif
	  #ifdef EBADF
		EBADF,
	  #endif
	  #ifdef EINTR
		EINTR,
	  #endif
	  #ifdef EIO
		EIO,
	  #endif
	  #ifdef ENOMEM
		ENOMEM,
	  #endif
	  #ifdef ENXIO
		ENXIO,
	  #endif
	  #ifdef EOVERFLOW
		EOVERFLOW,
	  #endif
	  #ifdef EILSEQ
		EILSEQ,
	  #endif
	};

mkwrap_body_errno_ferror("posix/stdio/sp/scanf", EOF, stdin)
mkwrap_variadic_bottom(scanf, vscanf, (format, arguments), format)
