
/*
 * libfiu remote control API
 */

#include <stdio.h>		/* snprintf() */
#include <string.h>		/* strncpy() */
#include <stdlib.h>		/* malloc()/free() */
#include <limits.h>		/* PATH_MAX */
#include <sys/types.h>		/* getpid(), mkfifo() */
#include <unistd.h>		/* getpid() */
#include <sys/stat.h>		/* mkfifo() */
#include <fcntl.h>		/* open() and friends */
#include <pthread.h>		/* pthread_create() and friends */
#include <errno.h>		/* errno and friends */

/* Enable us, so we get the real prototypes from the headers */
#define FIU_ENABLE 1

#include "fiu-control.h"
#include "internal.h"


/* Max length of a line containing a control directive */
#define MAX_LINE 512

/*
 * Generic remote control
 */

/* Reads a line from the given fd, assumes the buffer can hold MAX_LINE
 * characters. Returns the number of bytes read, or -1 on error. Inefficient,
 * but we don't really care. The final '\n' will not be included. */
static int read_line(int fd, char *buf)
{
	int r;
	char c;
	unsigned int len;

	c = '\0';
	len = 0;
	memset(buf, 0, MAX_LINE);

	do {
		r = read(fd, &c, 1);
		if (r < 0)
			return -1;
		if (r == 0)
			break;

		len += r;

		*buf = c;
		buf++;

	} while (c != '\n' && c != '\0' && len < MAX_LINE);

	if (len > 0 && c == '\n') {
		*(buf - 1) = '\0';
		len--;
	}

	return len;
}

/* Remote control command processing.
 * Supported commands:
 *  - disable <name>
 *  - enable <name> <failnum> <failinfo> [flags]
 *  - enable_random <name> <failnum> <failinfo> <probability> [flags]
 *
 * flags can be one of:
 *  - "one": same as FIU_ONETIME
 */
static int rc_process_cmd(char *cmd)
{
	char *tok, *state;
	char fp_name[MAX_LINE];
	int failnum;
	void *failinfo;
	float probability;
	unsigned int flags;

	state = NULL;

	tok = strtok_r(cmd, " ", &state);
	if (tok == NULL)
		return -1;

	if (strcmp(tok, "disable") == 0) {
		tok = strtok_r(NULL, " ", &state);
		if (tok == NULL)
			return -1;
		return fiu_disable(tok);

	} else if (strcmp(tok, "enable") == 0
			|| strcmp(tok, "enable_random") == 0) {

		tok = strtok_r(NULL, " ", &state);
		if (tok == NULL)
			return -1;
		strncpy(fp_name, tok, MAX_LINE);

		tok = strtok_r(NULL, " ", &state);
		if (tok == NULL)
			return -1;
		failnum = atoi(tok);

		tok = strtok_r(NULL, " ", &state);
		if (tok == NULL)
			return -1;
		failinfo = (void *) strtoul(tok, NULL, 10);

		probability = -1;
		if (strcmp(tok, "enable_random") == 0) {
			tok = strtok_r(NULL, " ", &state);
			if (tok == NULL)
				return -1;
			probability = strtod(tok, NULL);
			if (probability < 0 || probability > 1)
				return -1;
		}

		flags = 0;
		tok = strtok_r(NULL, " ", &state);
		if (tok != NULL) {
			if (strcmp(tok, "one") == 0)
				flags |= FIU_ONETIME;
		}

		if (probability >= 0) {
			return fiu_enable_random(fp_name, failnum, failinfo,
					probability, flags);
		} else {
			return fiu_enable(fp_name, failnum, failinfo, flags);
		}
	} else {
		return -1;
	}
}

/* Read remote control directives from fdr and process them, writing the
 * results in fdw. Returns the length of the line read, 0 if EOF, or < 0 on
 * error. */
static int rc_do_command(int fdr, int fdw)
{
	int len, r, reply_len;
	char buf[MAX_LINE], reply[MAX_LINE];

	len = read_line(fdr, buf);
	if (len <= 0)
		return len;

	r = rc_process_cmd(buf);

	reply_len = snprintf(reply, MAX_LINE, "%d\n", r);
	r = write(fdw, reply, reply_len);
	if (r <= 0)
		return r;

	return len;
}


/*
 * Remote control via named pipes
 *
 * Enables remote control over a named pipe that begins with the given
 * basename. "-$PID.in" will be appended to it to form the final path to read
 * commands from, and "-$PID.out" will be appended to it to form the final
 * path to write the replies to. After the process dies, the pipe will be
 * removed. If the process forks, a new pipe will be created.
 */

static char npipe_basename[PATH_MAX];
static char npipe_path_in[PATH_MAX];
static char npipe_path_out[PATH_MAX];

static void *rc_fifo_thread(void *unused)
{
	int fdr, fdw, r, errcount;

	/* increment the recursion count so we're not affected by libfiu,
	 * otherwise we could make the remote control useless by enabling all
	 * failure points */
	rec_count++;

	errcount = 0;

reopen:
	if (errcount > 10) {
		fprintf(stderr, "libfiu: Too many errors in remote control "
				"thread, shutting down\n");
		return NULL;
	}

	fdr = open(npipe_path_in, O_RDONLY);
	if (fdr < 0)
		return NULL;

	fdw = open(npipe_path_out, O_WRONLY);
	if (fdw < 0) {
		close(fdr);
		return NULL;
	}

	for (;;) {
		r = rc_do_command(fdr, fdw);
		if (r < 0 && errno != EPIPE) {
			perror("libfiu: Error reading from remote control");
			errcount++;
			close(fdr);
			close(fdw);
			goto reopen;
		} else if (r == 0 || (r < 0 && errno == EPIPE)) {
			/* one of the ends of the pipe was closed */
			close(fdr);
			close(fdw);
			goto reopen;
		}
	}

	/* we never get here */
}

static void fifo_atexit(void)
{
	unlink(npipe_path_in);
	unlink(npipe_path_out);
}

static int _fiu_rc_fifo(const char *basename)
{
	pthread_t thread;

	/* see rc_fifo_thread() */
	rec_count++;

	snprintf(npipe_path_in, PATH_MAX, "%s-%d.in", basename, getpid());
	snprintf(npipe_path_out, PATH_MAX, "%s-%d.out", basename, getpid());

	if (mkfifo(npipe_path_in, 0600) != 0 && errno != EEXIST) {
		rec_count--;
		return -1;
	}

	if (mkfifo(npipe_path_out, 0600) != 0 && errno != EEXIST) {
		unlink(npipe_path_in);
		rec_count--;
		return -1;
	}

	if (pthread_create(&thread, NULL, rc_fifo_thread, NULL) != 0) {
		unlink(npipe_path_in);
		unlink(npipe_path_out);
		rec_count--;
		return -1;
	}

	atexit(fifo_atexit);

	rec_count--;
	return 0;
}

static void fifo_atfork_child(void)
{
	_fiu_rc_fifo(npipe_basename);
}

int fiu_rc_fifo(const char *basename)
{
	int r;

	r = _fiu_rc_fifo(basename);
	if (r < 0)
		return r;

	strncpy(npipe_basename, basename, PATH_MAX);
	pthread_atfork(NULL, NULL, fifo_atfork_child);

	return r;
}


