
#ifndef _FIU_CODEGEN
#define _FIU_CODEGEN

#include <fiu.h>		/* fiu_* */
#include <stdlib.h>		/* NULL, random() */

/* Recursion counter, per-thread */
extern int __thread _fiu_called;

/* Get a symbol from libc */
void *libc_symbol(const char *symbol);

/* Some compilers support constructor priorities. Since we don't rely on them,
 * but use them for clarity purposes, use a macro so libfiu builds on systems
 * where they're not supported.
 * Compilers that are known to support constructor priorities:
 *  - GCC >= 4.3 on Linux
 *  - clang as of 2010-03-14
 */
#if \
	( (defined __GNUC__) \
		&& __GNUC__ >= 4 && __GNUC_MINOR__ >= 3 ) \
	|| (defined __clang__)
  #define constructor_attr(prio) __attribute__((constructor(prio)))
#else
  #define NO_CONSTRUCTOR_PRIORITIES 1
  #define constructor_attr(prio) __attribute__((constructor))
#endif

/* Useful macros for recursion and debugging */
#if 1
	#define rec_inc() do { _fiu_called++; } while(0)
	#define rec_dec() do { _fiu_called--; } while(0)
	#define printd(...) do { } while(0)

#else
	/* debug variants */
	#include <stdio.h>
	#include <unistd.h>

	#define rec_inc()				\
		do {					\
			_fiu_called++;			\
			fprintf(stderr, "I: %d %s\n", _fiu_called, \
					__FUNCTION__);	\
			fflush(stderr);			\
		} while (0)

	#define rec_dec()				\
		do {					\
			_fiu_called--;			\
			fprintf(stderr, "D: %d %s\n", _fiu_called, \
					__FUNCTION__);	\
			fflush(stderr);			\
		} while (0)

	#define printd(...)				\
		do {					\
			if (_fiu_called)		\
				fprintf(stderr, "\t");	\
			_fiu_called++;			\
			fprintf(stderr, "%6.6d ", getpid()); \
			fprintf(stderr, "%s(): ", __FUNCTION__ ); \
			fprintf(stderr, __VA_ARGS__);	\
			fflush(stderr);			\
			_fiu_called--;			\
		} while(0)
#endif


/*
 * Wrapper generator macros
 */

/* Generates the common top of the wrapped function */
#define mkwrap_top(RTYPE, NAME, PARAMS, PARAMSN, PARAMST, ON_ERR) \
	static RTYPE (*_fiu_orig_##NAME) PARAMS = NULL;		\
								\
	static int _fiu_in_init_##NAME = 0;			\
								\
	static void constructor_attr(201) _fiu_init_##NAME(void) \
	{							\
		rec_inc();					\
		_fiu_in_init_##NAME++;				\
								\
		_fiu_orig_##NAME = (RTYPE (*) PARAMST)		\
				libc_symbol(#NAME);		\
								\
		_fiu_in_init_##NAME--;				\
		rec_dec();					\
	}							\
								\
	RTYPE NAME PARAMS					\
	{ 							\
		RTYPE r;					\
		int fstatus;					\
								\
		if (_fiu_called) {				\
			if (_fiu_orig_##NAME == NULL) {		\
				if (_fiu_in_init_##NAME) {	\
					printd("fail on init\n"); \
					return ON_ERR;		\
				} else {			\
					printd("get orig\n");	\
					_fiu_init_##NAME();	\
				}				\
			}					\
			printd("orig\n");			\
			return (*_fiu_orig_##NAME) PARAMSN;	\
		}						\
								\
		printd("fiu\n");				\
								\
		/* fiu_fail() may call anything */		\
		rec_inc();


/* Generates the body of the function for normal, non-errno usage. The return
 * value is taken from failinfo. */
#define mkwrap_body_failinfo(FIU_NAME, RTYPE)			\
								\
		fstatus = fiu_fail(FIU_NAME);			\
		if (fstatus != 0) {				\
			r = (RTYPE) fiu_failinfo();		\
			goto exit;				\
		}

/* Generates the body of the function for normal, non-errno usage. The return
 * value is hardcoded. */
#define mkwrap_body_hardcoded(FIU_NAME, FAIL_RET)		\
								\
		fstatus = fiu_fail(FIU_NAME);			\
		if (fstatus != 0) {				\
			r = FAIL_RET;				\
			goto exit;				\
		}

/* Generates the body of the function for functions that affect errno. The
 * return value is hardcoded. Assumes int valid_errnos[] exist was properly
 * defined. */
#define mkwrap_body_errno(FIU_NAME, FAIL_RET) \
								\
		fstatus = fiu_fail(FIU_NAME);			\
		if (fstatus != 0) {				\
			void *finfo = fiu_failinfo();		\
			if (finfo == NULL) {			\
				errno = valid_errnos[random() % \
					sizeof(valid_errnos) / sizeof(int)]; \
			} else {				\
				errno = (long) finfo;		\
			}					\
			r = FAIL_RET;				\
			goto exit;				\
		}

/* Generates a body part that will reduce the CNT parameter in a random
 * amount when the given point of failure is enabled. Can be combined with the
 * other body generators. */
#define mkwrap_body_reduce(FIU_NAME, CNT)			\
								\
		fstatus = fiu_fail(FIU_NAME);			\
		if (fstatus != 0) {				\
			CNT -= random() % CNT;			\
		}

#define mkwrap_bottom(NAME, PARAMSN)				\
								\
		if (_fiu_orig_##NAME == NULL)			\
			_fiu_init_##NAME();			\
								\
		printd("calling orig\n");			\
		r = (*_fiu_orig_##NAME) PARAMSN;		\
								\
	exit:							\
		rec_dec();					\
		return r;					\
	}


#endif /* _FIU_CODEGEN */

