
#include <stdlib.h>		/* malloc() and friends */
#include <string.h>		/* strcmp() and friends */
#include <pthread.h>		/* mutexes */
#include <sys/time.h>		/* gettimeofday() */
#include <time.h>		/* gettimeofday() */
#include <limits.h>		/* ULONG_MAX */

/* Enable us, so we get the real prototypes from the headers */
#define FIU_ENABLE 1

#include "fiu.h"
#include "fiu-control.h"
#include "internal.h"
#include "wtable.h"

/* Tracing mode for debugging libfiu itself. */
#ifdef FIU_TRACE
  #include <stdio.h>
  #define trace(...) \
	do { fprintf(stderr, __VA_ARGS__); fflush(stderr); } while (0)
#else
  #define trace(...) do { } while(0)
#endif


/* Different methods to decide when a point of failure fails */
enum pf_method {
	PF_ALWAYS = 1,
	PF_PROB,
	PF_EXTERNAL,
	PF_STACK,
};

/* Point of failure information */
struct pf_info {
	char *name;
	unsigned int namelen;
	int failnum;
	void *failinfo;
	unsigned int flags;

	/* Lock and failed flag, used only when flags & FIU_ONETIME. */
	pthread_mutex_t lock;
	bool failed_once;

	/* How to decide when this point of failure fails, and the information
	 * needed to take the decision */
	enum pf_method method;
	union {
		/* To use when method == PF_PROB */
		float probability;

		/* To use when method == PF_EXTERNAL */
		external_cb_t *external_cb;

		/* To use when method == PF_STACK */
		struct stack {
			void *func_start;
			void *func_end;
			int func_pos_in_stack;
		} stack;
	} minfo;
};

/* Creates a new pf_info.
 * Only the common fields are filled, the caller should take care of the
 * method-specific ones. For internal use only. */
static struct pf_info *pf_create(const char *name, int failnum,
		void *failinfo, unsigned int flags, enum pf_method method)
{
	struct pf_info *pf;

	rec_count++;

	pf = malloc(sizeof(struct pf_info));
	if (pf == NULL)
		goto exit;

	pf->name = strdup(name);
	if (pf->name == NULL) {
		free(pf);
		pf = NULL;
		goto exit;
	}

	pf->namelen = strlen(name);
	pf->failnum = failnum;
	pf->failinfo = failinfo;
	pf->flags = flags;
	pf->method = method;

	/* These two are only use when flags & FIU_ONETIME. */
	pthread_mutex_init(&pf->lock, NULL);
	pf->failed_once = false;

exit:
	rec_count--;
	return pf;
}

static void pf_free(struct pf_info *pf) {
	free(pf->name);
	pthread_mutex_destroy(&pf->lock);
	free(pf);
}


/* Table used to keep the information about the enabled points of failure.
 * It is protected by enabled_fails_lock. */
wtable_t *enabled_fails = NULL;
static pthread_rwlock_t enabled_fails_lock = PTHREAD_RWLOCK_INITIALIZER;

#define ef_rlock() do { pthread_rwlock_rdlock(&enabled_fails_lock); } while (0)
#define ef_wlock() do { pthread_rwlock_wrlock(&enabled_fails_lock); } while (0)
#define ef_runlock() do { pthread_rwlock_unlock(&enabled_fails_lock); } while (0)
#define ef_wunlock() do { pthread_rwlock_unlock(&enabled_fails_lock); } while (0)


/* To prevent unwanted recursive calls that would deadlock, we use a
 * thread-local recursion count. Unwanted recursive calls can result from
 * using functions that have been modified to call fiu_fail(), which can
 * happen when using the POSIX preloader library: fiu_enable() takes the lock
 * for writing, and can call malloc() (for example), which can in turn call
 * fiu_fail() which can take the lock for reading.
 *
 * It is also modified at fiu-rc.c, to prevent failing within the remote
 * control thread.
 *
 * Sadly, we have to use the GNU extension for TLS, so we do not resort to
 * pthread_[get|set]specific() which could be wrapped. Luckily it's available
 * almost everywhere. */
__thread int rec_count = 0;


/* Used to keep the last failinfo via TLS */
static pthread_key_t last_failinfo_key;


/*
 * Miscelaneous internal functions
 */

/* Determines if the given address is within the function code. */
static int pc_in_func(struct pf_info *pf, void *pc)
{
	/* We don't know if the platform allows us to know func_end,
	 * so we use different methods depending on its availability. */
	if (pf->minfo.stack.func_end) {
		return (pc >= pf->minfo.stack.func_start &&
				pc <= pf->minfo.stack.func_end);
	} else {
		return pf->minfo.stack.func_start == get_func_start(pc);
	}
}

/* Determines wether to fail or not the given failure point, which is of type
 * PF_STACK. Returns 1 if it should fail, or 0 if it should not. */
static int should_stack_fail(struct pf_info *pf)
{
	// TODO: Find the right offset for pos_in_stack: we should look for
	// fiu_fail(), and start counting from there.
	int nptrs, i;
	void *buffer[100];

	nptrs = get_backtrace(buffer, 100);

	for (i = 0; i < nptrs; i++) {
		if (pc_in_func(pf, buffer[i]) &&
				(pf->minfo.stack.func_pos_in_stack == -1 ||
				 i == pf->minfo.stack.func_pos_in_stack)) {
			return 1;
		}
	}

	return 0;
}

/* Pseudorandom number generator.
 *
 * The performance of the PRNG is very sensitive to us, so we implement our
 * own instead of just use drand48() or similar.
 *
 * As we don't really need a very good, thread-safe or secure random source,
 * we use an algorithm similar to the one used in rand() and drand48() (a
 * linear congruential generator, see
 * http://en.wikipedia.org/wiki/Linear_congruential_generator for more
 * information). Coefficients are the ones used in rand(), so we assume
 * sizeof(int) >= 4.
 *
 * To seed it, we use the current microseconds. To prevent seed reuse, we
 * re-seed after each fork (see atfork_child()). */
static unsigned int randd_xn = 0xA673F42D;

static void prng_seed(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	randd_xn = tv.tv_usec;
}

static double randd(void)
{
	randd_xn = 1103515245 * randd_xn + 12345;

	return (double) randd_xn / UINT_MAX;
}

/* Function that runs after the process has been forked, at the child. It's
 * registered via pthread_atfork() in fiu_init(). */
static void atfork_child(void)
{
	prng_seed();
}


/*
 * Core API
 */

/* Initializes the library. It should be safe to call this more than once at
 * any time, to allow several independant libraries to use fiu at the same
 * time without clashes. */
int fiu_init(unsigned int flags)
{
	/* Used to avoid re-initialization, protected by enabled_fails_lock */
	static int initialized = 0;

	rec_count++;
	ef_wlock();
	if (initialized) {
		ef_wunlock();
		rec_count--;
		return 0;
	}

	pthread_key_create(&last_failinfo_key, NULL);

	enabled_fails = wtable_create((void (*)(void *)) pf_free);

	if (pthread_atfork(NULL, NULL, atfork_child) != 0) {
		ef_wunlock();
		rec_count--;
		return -1;
	}

	prng_seed();

	initialized = 1;

	ef_wunlock();
	rec_count--;
	return 0;
}

/* Returns the failure status of the given name. Must work well even before
 * fiu_init() is called assuming no points of failure are enabled; although it
 * can (and does) assume fiu_init() will be called before enabling any. */
int fiu_fail(const char *name)
{
	struct pf_info *pf;
	int failnum;

	rec_count++;

	/* We must do this before acquiring the lock and calling any
	 * (potentially wrapped) functions. */
	if (rec_count > 1) {
		rec_count--;
		return 0;
	}

	ef_rlock();

	/* It can happen that someone calls fiu_fail() before fiu_init(); we
	 * don't want to crash so we just exit. */
	if (enabled_fails == NULL) {
		goto exit;
	}

	pf = wtable_get(enabled_fails, name);

	/* None found. */
	if (pf == NULL) {
		goto exit;
	}

	if (pf->flags & FIU_ONETIME) {
		pthread_mutex_lock(&pf->lock);
		if (pf->failed_once) {
			pthread_mutex_unlock(&pf->lock);
			goto exit;
		}
		/* We leave it locked so we don't accidentally fail this
		 * point twice. */
	}

	switch (pf->method) {
		case PF_ALWAYS:
			goto exit_fail;
			break;
		case PF_PROB:
			if (pf->minfo.probability > randd())
				goto exit_fail;
			break;
		case PF_EXTERNAL:
			if (pf->minfo.external_cb(pf->name,
					&(pf->failnum),
					&(pf->failinfo),
					&(pf->flags)))
				goto exit_fail;
			break;
		case PF_STACK:
			if (should_stack_fail(pf))
				goto exit_fail;
			break;
		default:
			break;
	}

	if (pf->flags & FIU_ONETIME) {
		pthread_mutex_unlock(&pf->lock);
	}

exit:
	trace("FIU  Not failing %s\n", name);

	ef_runlock();
	rec_count--;
	return 0;

exit_fail:
	trace("FIU  Failing %s on %s\n", name, pf->name);

	pthread_setspecific(last_failinfo_key,
			pf->failinfo);
	failnum = pf->failnum;

	if (pf->flags & FIU_ONETIME) {
		pf->failed_once = true;
		pthread_mutex_unlock(&pf->lock);
	}

	ef_runlock();
	rec_count--;
	return failnum;
}

/* Returns the information associated with the last fail. */
void *fiu_failinfo(void)
{
	return pthread_getspecific(last_failinfo_key);
}


/*
 * Control API
 */

/* Inserts a pf into in the enabled_fails table.
 * Returns 0 on success, -1 on failure. */
static int insert_pf(struct pf_info *pf)
{
	bool success;

	rec_count++;

	ef_wlock();
	success = wtable_set(enabled_fails, pf->name, pf);
	ef_wunlock();

	rec_count--;
	return success ? 0 : -1;
}

/* Makes the given name fail. */
int fiu_enable(const char *name, int failnum, void *failinfo,
		unsigned int flags)
{
	struct pf_info *pf;

	pf = pf_create(name, failnum, failinfo, flags, PF_ALWAYS);
	if (pf == NULL)
		return -1;

	return insert_pf(pf);
}

/* Makes the given name fail with the given probability. */
int fiu_enable_random(const char *name, int failnum, void *failinfo,
		unsigned int flags, float probability)
{
	struct pf_info *pf;

	pf = pf_create(name, failnum, failinfo, flags, PF_PROB);
	if (pf == NULL)
		return -1;

	pf->minfo.probability = probability;
	return insert_pf(pf);
}

/* Makes the given name fail when the external function returns != 0. */
int fiu_enable_external(const char *name, int failnum, void *failinfo,
		unsigned int flags, external_cb_t *external_cb)
{
	struct pf_info *pf;

	pf = pf_create(name, failnum, failinfo, flags, PF_EXTERNAL);
	if (pf == NULL)
		return -1;

	pf->minfo.external_cb = external_cb;
	return insert_pf(pf);
}

/* Makes the given name fail when func is in the stack at func_pos.
 * If func_pos is -1, then any position will match. */
int fiu_enable_stack(const char *name, int failnum, void *failinfo,
		unsigned int flags, void *func, int func_pos_in_stack)
{
	struct pf_info *pf;

	/* Specifying the stack position is unsupported for now */
	if (func_pos_in_stack != -1)
		return -1;

	if (backtrace_works((void (*)()) fiu_enable_stack) == 0)
		return -1;

	// We need either get_func_end() or get_func_start() to work, see
	// pc_in_func() above.
	if (get_func_end(func) == NULL && get_func_start(func) == NULL)
		return -1;

	pf = pf_create(name, failnum, failinfo, flags, PF_STACK);
	if (pf == NULL)
		return -1;

	pf->minfo.stack.func_start = func;
	pf->minfo.stack.func_end = get_func_end(func);
	pf->minfo.stack.func_pos_in_stack = func_pos_in_stack;
	return insert_pf(pf);
}

/* Same as fiu_enable_stack(), but takes a function name. */
int fiu_enable_stack_by_name(const char *name, int failnum, void *failinfo,
		unsigned int flags, const char *func_name,
		int func_pos_in_stack)
{
	void *fp;

	/* We need to check this here instead of relying on the test within
	 * fiu_enable_stack() in case it is inlined; that would fail the check
	 * because fiu_enable_stack() would not be in the stack. */
	if (backtrace_works((void (*)()) fiu_enable_stack_by_name) == 0)
		return -1;

	fp = get_func_addr(func_name);
	if (fp == NULL)
		return -1;

	return fiu_enable_stack(name, failnum, failinfo, flags, fp,
			func_pos_in_stack);
}

/* Makes the given name NOT fail. */
int fiu_disable(const char *name)
{
	bool success;

	rec_count++;

	/* Just find the point of failure and remove it. */
	ef_wlock();
	success = wtable_del(enabled_fails, name);
	ef_wunlock();

	rec_count--;
	return success ? 0 : -1;
}


