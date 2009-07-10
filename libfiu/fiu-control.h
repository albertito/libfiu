
#ifndef _FIU_CONTROL_H
#define _FIU_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Control API for libfiu
 */

/* Flags for fiu_enable*() */
#define FIU_ONETIME 1		/* Only fail once */


/* Enables the given point of failure. That makes it always fail.
 *
 * - name: point of failure name.
 * - failnum: what will fiu_fail() return, must be != 0.
 * - failinfo: what will fiu_failinfo() return.
 * - flags: flags.
 * - returns: 0 if success, < 0 otherwise.
 */
int fiu_enable(const char *name, int failnum, void *failinfo,
		unsigned int flags);


/* Enables the given point of failure, with the given probability. That makes
 * it fail with the given probablity.
 *
 * - name: point of failure name.
 * - failnum: what will fiu_fail() return, must be != 0.
 * - failinfo: what will fiu_failinfo() return.
 * - flags: flags.
 * - probability: probability a fiu_fail() call will return failnum,
 * 	between 0 (never fail) and 1 (always fail). As a special fast case, -1
 * 	can also be used to always fail.
 * - returns: 0 if success, < 0 otherwise. */
int fiu_enable_random(const char *name, int failnum, void *failinfo,
		unsigned int flags, float probability);


/** Type of external callback functions. They must return 0 to indicate not to
 * fail, != 0 to indicate otherwise. Can modify failnum, failinfo and flags,
 * in order to alter the values of the point of failure. */
typedef int external_cb_t(const char *name, int *failnum, void **failinfo,
		unsigned int *flags);

/* Enables the given point of failure, leaving the decision whether to fail or
 * not to the given external function.
 *
 * - name: point of failure name.
 * - failnum: what will fiu_fail() return, must be != 0.
 * - failinfo: what will fiu_failinfo() return.
 * - flags: flags.
 * - external_cb: function to call to determine whether fail or not. */
int fiu_enable_external(const char *name, int failnum, void *failinfo,
		unsigned int flags, external_cb_t *external_cb);

/* Disables the given point of failure. That makes it NOT fail.
 *
 * - name: point of failure name.
 * - returns: 0 if success, < 0 otherwise. */
int fiu_disable(const char *name);

/* Enables remote control over a named pipe that begins with the given
 * basename. "-$PID" will be appended to it to form the final path. After the
 * process dies, the pipe will be removed. If the process forks, a new pipe
 * will be created.
 *
 * - basename: base path to use in the creation of the named pipes.
 * - returns: 0 on success, -1 on errors. */
int fiu_rc_fifo(const char *basename);


#ifdef __cplusplus
}
#endif

#endif // _FIU_CONTROL_H

