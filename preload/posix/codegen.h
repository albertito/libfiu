
#ifndef _FIU_CODEGEN
#define _FIU_CODEGEN

#include <dlfcn.h>		/* dlsym() */
#include <fiu.h>		/* fiu_* */
#include <stdlib.h>		/* NULL, random() */

/* Pointer to the dynamically loaded library */
extern void *_fiu_libc;

/* Recursion counter, per-thread */
extern int __thread _fiu_called;

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
			fprintf(stderr, "I: %d\n", _fiu_called); \
			fflush(stderr);			\
		} while (0)

	#define rec_dec()				\
		do {					\
			_fiu_called--;			\
			fprintf(stderr, "D: %d\n", _fiu_called); \
			fflush(stderr);			\
		} while (0)

	#define printd(...)				\
		do {					\
			if (_fiu_called)		\
				fprintf(stderr, "\t");	\
			_fiu_called++;			\
			fprintf(stderr, "%5.5d ", getpid()); \
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
#define mkwrap_top(RTYPE, NAME, PARAMS, PARAMSN, PARAMST)	\
	static RTYPE (*_fiu_orig_##NAME) PARAMS = NULL;		\
								\
	static void __attribute__((constructor(201))) _fiu_init_##NAME(void) \
	{							\
		rec_inc();					\
		_fiu_orig_##NAME = (RTYPE (*) PARAMST)		\
				dlsym(_fiu_libc, #NAME);	\
		rec_dec();					\
	}							\
								\
	RTYPE NAME PARAMS					\
	{ 							\
		RTYPE r;					\
		int fstatus;					\
								\
		if (_fiu_called) {				\
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
#define mkwrap_body_errno(FIU_NAME, FAIL_RET, NVERRNOS) \
								\
		fstatus = fiu_fail(FIU_NAME);			\
		if (fstatus != 0) {				\
			void *finfo = fiu_failinfo();		\
			if (finfo == NULL) {			\
				errno = valid_errnos[random() % NVERRNOS]; \
			} else {				\
				errno = (long) finfo;		\
			}					\
			r = FAIL_RET;				\
			goto exit;				\
		}


#define mkwrap_bottom(NAME, PARAMSN)				\
								\
		r = (*_fiu_orig_##NAME) PARAMSN;		\
								\
	exit:							\
		rec_dec();					\
		return r;					\
	}


#endif /* _FIU_CODEGEN */

