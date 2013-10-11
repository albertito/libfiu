
/* Test creation and removal of failure points while checking them on a
 * different thread.
 *
 * Please note this is a non-deterministic test. */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include <fiu.h>
#include <fiu-control.h>

#define NPOINTS 10000

/* How many seconds to run the test */
#define TEST_TIME 5

/* Signal for the threads to stop. */
bool stop_threads = false;

char *make_point_name(char *name, int i)
{
	sprintf(name, "fp-%d", i);
	return name;
}

/* Calls all the points all the time. */
void *no_check_caller(void *unused)
{
	int i;
	char name[200];

	while (!stop_threads) {
		for (i = 0; i < NPOINTS; i++) {
			fiu_fail(make_point_name(name, i));
		}
	}

	return NULL;
}

bool rand_bool(void) {
	return (rand() % 2) == 0;
}

/* Used too know if a point is enabled or not. */
bool enabled[NPOINTS];
pthread_mutex_t enabled_lock = PTHREAD_MUTEX_INITIALIZER;



/* Calls all the *enabled* points all the time. */
void *checking_caller(void *unused)
{
	int i = 0;
	int failed;
	char name[200];

	while (!stop_threads) {
		pthread_mutex_lock(&enabled_lock);
		if (enabled[i]) {
			failed = fiu_fail(make_point_name(name, i)) != 0;
			if (!failed) {
				printf("ERROR: fp %d did not fail\n", i);
				assert(false);
			}
		}
		pthread_mutex_unlock(&enabled_lock);

		i = (i + 1) % NPOINTS;
	}

	return NULL;
}

/* Enable and disable points all the time. */
void *enabler(void *unused)
{
	int i = 0;
	char name[200];

	while (!stop_threads) {
		pthread_mutex_lock(&enabled_lock);
		if (rand_bool()) {
			make_point_name(name, i);
			if (enabled[i]) {
				assert(fiu_disable(name) == 0);
				enabled[i] = false;
			} else {
				assert(fiu_enable(name, 1, NULL, 0) == 0);
				enabled[i] = true;
			}
		}
		pthread_mutex_unlock(&enabled_lock);

		i = (i + 1) % NPOINTS;
	}

	return NULL;
}

void disable_all()
{
	int i = 0;
	char name[200];

	pthread_mutex_lock(&enabled_lock);
	for (i = 0; i < NPOINTS; i++) {
		/* Note this could fail as we don't check if they're active or
		 * not, but here we don't care. */
		fiu_disable(make_point_name(name, i));
	}
	pthread_mutex_unlock(&enabled_lock);
}

int main(void)
{
	pthread_t t1, t2, t3;

	fiu_init(0);

	pthread_create(&t1, NULL, no_check_caller, NULL);
	pthread_create(&t2, NULL, checking_caller, NULL);
	pthread_create(&t3, NULL, enabler, NULL);

	sleep(TEST_TIME);

	stop_threads = 1;

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);

	disable_all();

	return 0;
}

