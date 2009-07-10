
/* libfiu - Fault Injection in Userspace
 *
 * This header, part of libfiu, contains the API that your project should use.
 *
 * If you want to avoid having libfiu as a mandatory build-time dependency,
 * you should include fiu-stubs.h in your project, and #include it instead of
 * this.
 */

#ifndef _FIU_H
#define _FIU_H


/* Controls whether the external code enables libfiu or not. */
#ifdef FIU_ENABLE

#ifdef __cplusplus
extern "C" {
#endif


/* Initializes the library.
 *
 * - flags: unused.
 * - returns: 0 if success, < 0 if error. */
int fiu_init(unsigned int flags);

/* Returns the failure status of the given point of failure.
 *
 * - name: point of failure name.
 * - returns: the failure status (0 means it should not fail). */
int fiu_fail(const char *name);

/* Returns the information associated with the last failure.
 *
 * - returns: the information associated with the last fail, or NULL if there
 *      isn't one. */
void *fiu_failinfo(void);

/* Performs the given action when the given point of failure fails. Mostly
 * used in the following macros. */
#define fiu_do_on(name, action) \
        do { \
                if (fiu_fail(name)) { \
                        action; \
                } \
        } while (0)

/* Exits the program when the given point of failure fails. */
#define fiu_exit_on(name) fiu_do_on(name, exit(EXIT_FAILURE))

/* Makes the function return the given retval when the given point of failure
 * fails. */
#define fiu_return_on(name, retval) fiu_do_on(name, return retval)


#ifdef __cplusplus
}
#endif

#else
/* fiu not enabled, this should match fiu-local.h but we don't include it
 * because it includes us assuming we're installed system-wide */

#define fiu_init(flags) 0
#define fiu_fail(name) 0
#define fiu_failinfo() NULL
#define fiu_do_on(name, action)
#define fiu_exit_on(name)
#define fiu_return_on(name, retval)

#endif /* FIU_ENABLE */

#endif /* _FIU_H */

