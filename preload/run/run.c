
#include <stdio.h>		/* printf() */
#include <unistd.h>		/* execve() */
#include <string.h>		/* strtok(), memset(), strncpy() */
#include <stdlib.h>		/* atoi(), atol() */
#include <getopt.h>		/* getopt() */

#include <fiu.h>
#include <fiu-control.h>


static void __attribute__((constructor)) fiu_run_init(void)
{
	char *fiu_fifo_env, *fiu_enable_env;

	fiu_init(0);

	fiu_fifo_env = getenv("FIU_CTRL_FIFO");
	if (fiu_fifo_env && *fiu_fifo_env != '\0') {
		if (fiu_rc_fifo(fiu_fifo_env) < 0) {
			perror("fiu_run_preload: Error opening RC fifo");
		}
	}

	fiu_enable_env = getenv("FIU_ENABLE");
	if (fiu_enable_env && *fiu_enable_env != '\0') {
		/* FIU_ENABLE can contain more than one command, separated by
		 * a newline, so we split them and call fiu_rc_string()
		 * accordingly. */
		char *tok, *state;
		char *env_copy;
		char *rc_error;

		env_copy = strdup(fiu_enable_env);
		if (env_copy == NULL) {
			perror("fiu_run_preload: Error in strdup()");
			return;
		}

		tok = strtok_r(env_copy, "\n", &state);
		while (tok) {
			if (fiu_rc_string(tok, &rc_error) != 0) {
				fprintf(stderr,
					"fiu_run_preload: Error applying "
						"FIU_ENABLE commands: %s\n",
						rc_error);
				return;
			}
			tok = strtok_r(NULL, "\n", &state);
		}
	}
}

