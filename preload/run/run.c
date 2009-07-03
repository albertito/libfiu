
#include <stdio.h>		/* printf() */
#include <unistd.h>		/* execve() */
#include <string.h>		/* strtok(), memset(), strncpy() */
#include <stdlib.h>		/* atoi(), atol() */
#include <getopt.h>		/* getopt() */

#include <fiu.h>
#include <fiu-control.h>

/* Maximum size of parameters to the enable options */
#define MAX_ENOPT 128

enum fp_type {
	FP_ALWAYS = 1,
	FP_PROB = 2,
};

struct enable_option {
	char buf[MAX_ENOPT];
	enum fp_type type;
	char *name;
	int failnum;
	unsigned long failinfo;
	float probability;
};

/* Fills the enopt structure taking the values from the given string, which is
 * assumed to be in of the form:
 *
 *   name,probability,failnum,failinfo
 *
 * All fields are optional, except for the name. On error, enopt->name will be
 * set to NULL. Probability is in percent, -1 indicates it should always be
 * enabled. */
static void parse_enable(const char *s, struct enable_option *enopt)
{
	char *tok;
	char *state;

	enopt->name = NULL;
	enopt->type = FP_ALWAYS;
	enopt->failnum = 1;
	enopt->failinfo = 0;
	enopt->probability = 1;

	memset(enopt->buf, 0, MAX_ENOPT);
	strncpy(enopt->buf, s, MAX_ENOPT);

	tok = strtok_r(enopt->buf, ",", &state);
	if (tok == NULL)
		return;
	enopt->name = tok;

	tok = strtok_r(NULL, ",", &state);
	if (tok == NULL)
		return;
	if (atoi(tok) >= 0) {
		enopt->type = FP_PROB;
		enopt->probability = atof(tok) / 100.0;
	}

	tok = strtok_r(NULL, ",", &state);
	if (tok == NULL)
		return;
	enopt->failnum = atoi(tok);

	tok = strtok_r(NULL, ",", &state);
	if (tok == NULL)
		return;
	enopt->failinfo = atol(tok);
}

static void __attribute__((constructor)) fiu_run_init(void)
{
	int r;
	struct enable_option enopt;
	char *fiu_enable_env, *fiu_enable_s, *fiu_fifo_env;
	char *tok, *state;

	fiu_init(0);

	fiu_fifo_env = getenv("FIU_CTRL_FIFO");
	if (fiu_fifo_env != NULL && *fiu_fifo_env != '\0') {
		if (fiu_rc_fifo(fiu_fifo_env) < 0) {
			perror("fiu_run_preload: Error opening RC fifo");
		}
	}

	fiu_enable_env = getenv("FIU_ENABLE");
	if (fiu_enable_env == NULL)
		return;

	/* copy fiu_enable_env to fiu_enable_s so we can strtok() it */
	fiu_enable_s = malloc(strlen(fiu_enable_env) + 1);
	strcpy(fiu_enable_s, fiu_enable_env);

	state = NULL;
	tok = strtok_r(fiu_enable_s, ":", &state);
	while (tok != NULL) {
		parse_enable(tok, &enopt);
		if (enopt.name == NULL) {
			fprintf(stderr, "fiu-run.so: "
					"ignoring enable without name");
			tok = strtok_r(NULL, ":", &state);
			continue;
		}

		if (enopt.type == FP_ALWAYS) {
			r = fiu_enable(enopt.name, enopt.failnum,
					(void *) enopt.failinfo, 0);
		} else {
			r = fiu_enable_random(enopt.name, enopt.failnum,
					(void *) enopt.failinfo, 0,
					enopt.probability);
		}

		if (r < 0) {
			fprintf(stderr, "fiu-run.so: "
					"couldn't enable failure %s\n",
					enopt.name);
			continue;
		}

		tok = strtok_r(NULL, ":", &state);
	}

	free(fiu_enable_s);
}

