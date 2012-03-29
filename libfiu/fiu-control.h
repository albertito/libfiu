
/** @file
 *
 * This header contains the control API.
 * It should be used for controlling the injection of failures, usually when
 * writing tests.
 */

#ifndef _FIU_CONTROL_H
#define _FIU_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif


/* Flags for fiu_enable*() */

/** Only fail once; the point of failure will be automatically disabled
 * afterwards. */
#define FIU_ONETIME 1


/** Enables the given point of failure unconditionally.
 *
 * @param name  Name of the point of failure to enable.
 * @param failnum  What will fiu_fail() return, must be != 0.
 * @param failinfo  What will fiu_failinfo() return.
 * @param flags  Flags.
 * @returns 0 if success, < 0 otherwise.
 */
int fiu_enable(const char *name, int failnum, void *failinfo,
		unsigned int flags);


/** Enables the given point of failure, with the given probability. That makes
 * it fail with the given probability.
 *
 * @param name  Name of the point of failure to enable.
 * @param failnum  What will fiu_fail() return, must be != 0.
 * @param failinfo  What will fiu_failinfo() return.
 * @param flags  Flags.
 * @param probability Probability a fiu_fail() call will return failnum,
 * 	between 0 (never fail) and 1 (always fail). As a special fast case, -1
 * 	can also be used to always fail.
 * @returns  0 if success, < 0 otherwise.
 */
int fiu_enable_random(const char *name, int failnum, void *failinfo,
		unsigned int flags, float probability);


/** Type of external callback functions.
 * They must return 0 to indicate not to fail, != 0 to indicate otherwise. Can
 * modify failnum, failinfo and flags, in order to alter the values of the
 * point of failure. */
typedef int external_cb_t(const char *name, int *failnum, void **failinfo,
		unsigned int *flags);

/** Enables the given point of failure, leaving the decision whether to fail
 * or not to the given external function.
 *
 * @param name  Name of the point of failure to enable.
 * @param failnum  What will fiu_fail() return, must be != 0.
 * @param failinfo  What will fiu_failinfo() return.
 * @param flags  Flags.
 * @param external_cb  Function to call to determine whether to fail or not.
 * @returns  0 if success, < 0 otherwise.
 */
int fiu_enable_external(const char *name, int failnum, void *failinfo,
		unsigned int flags, external_cb_t *external_cb);

/* Enables the given point of failure, but only if the given function is in
 * the stack at the given position.
 *
 * This function relies on GNU extensions such as backtrace() and dladdr(), so
 * it may not be available on your platform.
 *
 * - name: point of failure name.
 * - failnum: what will fiu_fail() return, must be != 0.
 * - failinfo: what will fiu_failinfo() return.
 * - flags: flags.
 * - func: pointer to the function.
 * - func_pos: position where we expect the function to be; use -1 for "any".
 * 	Values other than -1 are not supported at the moment, but will be in
 * 	the future.
 */
int fiu_enable_stack(const char *name, int failnum, void *failinfo,
		unsigned int flags, void *func, int func_pos_in_stack);

/** Enables the given point of failure, but only if 'func_name' is in
 * the stack at 'func_pos_in_stack'.
 *
 * This function relies on GNU extensions such as backtrace() and dladdr(), so
 * if your platform does not support them, it will always return failure.
 *
 * It is exactly like fiu_enable_stack() but takes a function name, it will
 * ask the dynamic linker to find the corresponding function pointer.
 *
 * @param name  Name of the point of failure to enable.
 * @param failnum  What will fiu_fail() return, must be != 0.
 * @param failinfo  What will fiu_failinfo() return.
 * @param flags  Flags.
 * @param func_name  Name of the function.
 * @param func_pos_in_stack  Position where we expect the function to be; use
 * 		-1 for "any". Values other than -1 are not supported at the
 * 		moment, but will be in the future.
 * @returns  0 if success, < 0 otherwise.
 */
int fiu_enable_stack_by_name(const char *name, int failnum, void *failinfo,
		unsigned int flags, const char *func_name,
		int func_pos_in_stack);

/** Disables the given point of failure. That makes it NOT fail.
 *
 * @param name  Name of the point of failure to disable.
 * @returns  0 if success, < 0 otherwise.
 */
int fiu_disable(const char *name);

/** Enables remote control over a named pipe.
 *
 * The name pipe path will begin with the given basename. "-$PID" will be
 * appended to it to form the final path. After the process dies, the pipe
 * will be removed. If the process forks, a new pipe will be created.
 *
 * Once this function has been called, the fiu-ctrl utility can be used to
 * control the points of failure externally.
 *
 * @param basename  Base path to use in the creation of the named pipes.
 * @returns  0 on success, -1 on errors. */
int fiu_rc_fifo(const char *basename);

/** Applies a remote control command given via a string.
 *
 * The format of the string is not stable and is still subject to change.
 * At the moment, this function is exported for consumption by the libfiu
 * utilities.
 *
 * @param cmd:  A zero-terminated string with the command to apply.
 * @param error:  In case of an error, it will point to a human-readable error
 *			message.
 * @returns  0 if success, < 0 otherwise.
 */
int fiu_rc_string(const char *cmd, char ** const error);


#ifdef __cplusplus
}
#endif

#endif // _FIU_CONTROL_H

