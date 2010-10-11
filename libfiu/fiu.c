
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


/* Different methods to decide when a point of failure fails */
enum pf_method {
	PF_ALWAYS = 1,
	PF_PROB,
	PF_EXTERNAL,
};

/* Point of failure information */
struct pf_info {
	char *name;
	unsigned int namelen;
	int failnum;
	void *failinfo;
	unsigned int flags;

	/* How to decide when this point of failure fails, and the information
	 * needed to take the decision */
	enum pf_method method;
	union {
		/* To use when method == PF_PROB */
		float probability;

		/* To use when method == PF_EXTERNAL */
		external_cb_t *external_cb;

	} minfo;
};


/* Array used to keep the information about the enabled points of failure.
 * It's an array because we assume it's going to be short enough for the
 * linear lookup not matter.
 * In the future, if it turns out it's normal that it grows large enough, we
 * may be interested in a more sophisticated structure like a hash table
 * and/or a bloom filter. */
static struct pf_info *enabled_fails = NULL;
static struct pf_info *enabled_fails_last = NULL;
static size_t enabled_fails_len = 0;
static size_t enabled_fails_nfree = 0;
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


/* Maximum number of free elements in enabled_fails (used to decide when to
 * shrink). */
#define EF_MAX_FREE 3

/* How much to grow enabled_fails by each time, it's recommended that this is
 * less than EF_MAX_FREE. */
#define EF_GROW 2


/* Used to keep the last failinfo via TLS */
static pthread_key_t last_failinfo_key;


/*
 * Miscelaneous internal functions
 */

/* Disables the given pf_info, assuming it's inside enabled_fails. Must be
 * called with enabled_fails_lock acquired. */
static void disable_pf(struct pf_info *pf)
{
	/* free the name we've allocated in setup_fail() via strdup() */
	free(pf->name);
	pf->name = NULL;
	pf->namelen = 0;
	pf->failnum = 0;
	pf->failinfo = NULL;
	pf->flags = 0;
}

/* Return the last position where s1 and s2 match. */
static unsigned int strlast(const char *s1, const char *s2)
{
	unsigned int i = 0;

	while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
		i++;
		s1++;
		s2++;
	}

	return i;
}

/* Checks if pf's name matches the one given. pf->name can be NULL. */
static int name_matches(const struct pf_info *pf, const char *name, int exact)
{
	if (pf->name == NULL || name == NULL)
		return 0;

	if (exact || pf->name[pf->namelen - 1] != '*')
		return strcmp(pf->name, name) == 0;

	/* Inexact match */
	return strlast(pf->name, name) >= pf->namelen - 1;
}

/* Shrink enabled_fails, used when it has too many free elements. Must be
 * called with enabled_fails_lock acquired. */
static int shrink_enabled_fails(void)
{
	int i;
	size_t new_len;
	struct pf_info *new, *pf;

	new_len = enabled_fails_len - enabled_fails_nfree + EF_GROW;

	new = malloc(new_len * sizeof(struct pf_info));
	if (new == NULL)
		return -1;

	i = 0;
	for (pf = enabled_fails; pf <= enabled_fails_last; pf++) {
		if (pf->name == NULL)
			continue;

		memcpy(new + i, pf, sizeof(struct pf_info));
		i++;
	}

	memset(new + i, 0, (new_len - i) * sizeof(struct pf_info));

	free(enabled_fails);
	enabled_fails = new;
	enabled_fails_len = new_len;
	enabled_fails_last = new + new_len - 1;
	enabled_fails_nfree = EF_GROW;

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

	enabled_fails = NULL;
	enabled_fails_last = NULL;
	enabled_fails_len = 0;
	enabled_fails_nfree = 0;

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

	/* we must do this before acquiring the lock and calling any
	 * (potentially wrapped) functions */
	if (rec_count > 1) {
		rec_count--;
		return 0;
	}

	ef_rlock();

	if (enabled_fails == NULL) {
		ef_runlock();
		rec_count--;
		return 0;
	}

	for (pf = enabled_fails; pf <= enabled_fails_last; pf++) {
		if (name_matches(pf, name, 0)) {
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
			default:
				break;
			}

			break;
		}
	}

	ef_runlock();
	rec_count--;
	return 0;

exit_fail:
	pthread_setspecific(last_failinfo_key,
			pf->failinfo);
	failnum = pf->failnum;

	if (pf->flags & FIU_ONETIME) {
		disable_pf(pf);
		enabled_fails_nfree++;
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

/* Sets up the given pf. For internal use only. */
static int setup_fail(struct pf_info *pf, const char *name, int failnum,
		void *failinfo, unsigned int flags, enum pf_method method,
		float probability, external_cb_t *external_cb)
{
	pf->name = strdup(name);
	pf->namelen = strlen(name);
	pf->failnum = failnum;
	pf->failinfo = failinfo;
	pf->flags = flags;
	pf->method = method;
	switch (method) {
		case PF_ALWAYS:
			break;
		case PF_PROB:
			pf->minfo.probability = probability;
			break;
		case PF_EXTERNAL:
			pf->minfo.external_cb = external_cb;
			break;
		default:
			return -1;
	}

	if (pf->name == NULL)
		return -1;
	return 0;

}

/* Creates a new pf in the enabled_fails table. For internal use only. */
static int insert_new_fail(const char *name, int failnum, void *failinfo,
		unsigned int flags, enum pf_method method, float probability,
		external_cb_t *external_cb)
{
	struct pf_info *pf;
	int rv = -1;
	size_t prev_len;

	rec_count++;

	/* See if it's already there and update the data if so, or if we have
	 * a free spot where to put it */
	ef_wlock();
	if (enabled_fails != NULL && enabled_fails_nfree > 0) {
		for (pf = enabled_fails; pf <= enabled_fails_last; pf++) {
			if (pf->name == NULL || strcmp(pf->name, name) == 0) {
				rv = setup_fail(pf, name, failnum, failinfo,
						flags, method, probability,
						external_cb);
				if (rv == 0)
					enabled_fails_nfree--;

				goto exit;
			}
		}

		/* There should be a free slot, but couldn't find one! This
		 * shouldn't happen */
		rv = -1;
		goto exit;
	}

	/* There are no free slots in enabled_fails, so we must grow it */
	enabled_fails = realloc(enabled_fails,
			(enabled_fails_len + EF_GROW) * sizeof(struct pf_info));
	if (enabled_fails == NULL) {
		enabled_fails_last = NULL;
		enabled_fails_len = 0;
		enabled_fails_nfree = 0;
		rv = -1;
		goto exit;
	}

	prev_len = enabled_fails_len;
	enabled_fails_len += EF_GROW;
	enabled_fails_nfree = EF_GROW;

	memset(enabled_fails + prev_len, 0,
			EF_GROW * sizeof(struct pf_info));

	enabled_fails_last = enabled_fails + enabled_fails_len - 1;

	pf = enabled_fails + prev_len;
	rv = setup_fail(pf, name, failnum, failinfo, flags, method,
			probability, external_cb);
	if (rv == 0)
		enabled_fails_nfree--;

exit:
	ef_wunlock();
	rec_count--;
	return rv;
}

/* Makes the given name fail. */
int fiu_enable(const char *name, int failnum, void *failinfo,
		unsigned int flags)
{
	return insert_new_fail(name, failnum, failinfo, flags, PF_ALWAYS, 0,
			NULL);
}

/* Makes the given name fail with the given probability. */
int fiu_enable_random(const char *name, int failnum, void *failinfo,
		unsigned int flags, float probability)
{
	return insert_new_fail(name, failnum, failinfo, flags, PF_PROB,
			probability, NULL);
}

/* Makes the given name fail when the external function returns != 0. */
int fiu_enable_external(const char *name, int failnum, void *failinfo,
		unsigned int flags, external_cb_t *external_cb)
{
	return insert_new_fail(name, failnum, failinfo, flags, PF_EXTERNAL,
			0, external_cb);
}

/* Makes the given name NOT fail. */
int fiu_disable(const char *name)
{
	struct pf_info *pf;

	rec_count++;

	/* just find the point of failure and mark it as free by setting its
	 * name to NULL */
	ef_wlock();

	if (enabled_fails == NULL) {
		ef_wunlock();
		rec_count--;
		return -1;
	}

	for (pf = enabled_fails; pf <= enabled_fails_last; pf++) {
		if (name_matches(pf, name, 1)) {
			disable_pf(pf);
			enabled_fails_nfree++;
			if (enabled_fails_nfree > EF_MAX_FREE)
				shrink_enabled_fails();
			ef_wunlock();
			rec_count--;
			return 0;
		}
	}

	ef_wunlock();
	rec_count--;
	return -1;
}


