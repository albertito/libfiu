
include: <unistd.h>
include: <errno.h>
include: <sys/types.h>
include: <sys/wait.h>
include: <signal.h>

fiu name base: posix/proc/

pid_t fork(void);
	on error: -1
	valid errnos: EAGAIN ENOMEM

pid_t wait(int *status);
	on error: -1
	valid errnos: ECHILD EINTR EINVAL

pid_t waitpid(pid_t pid, int *status, int options);
	on error: -1
	valid errnos: ECHILD EINTR EINVAL

# FreeBSD does not have waitid()
v: #ifndef __FreeBSD__
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
	on error: -1
	valid errnos: ECHILD EINTR EINVAL
v: #endif

int kill(pid_t pid, int sig);
	on error: -1
	valid errnos: EINVAL EPERM ESRCH

# We need to do this typedef because our parser is not smart enough to handle
# the function definition without it
v: typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);
	on error: SIG_ERR
        valid errnos: EINVAL

int sigaction(int signum, const struct sigaction *act, \
		struct sigaction *oldact);
	on error: -1
	valid errnos: EFAULT EINVAL

