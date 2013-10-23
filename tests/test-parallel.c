
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

/* The name of each point */
char point_name[NPOINTS][16];

char *make_point_name(char *name, int i)
{
	sprintf(name, "fp-%d", i);
	return name;
}

/* Calls all the points all the time. */
unsigned long long no_check_caller_count = 0;
void *no_check_caller(void *unused)
{
	int i = 0;

	while (!stop_threads) {
		fiu_fail(point_name[i]);

		i = (i + 1) % NPOINTS;

		if (i == 0)
			no_check_caller_count++;
	}

	no_check_caller_count = no_check_caller_count * NPOINTS + i;

	return NULL;
}

bool rand_bool(void) {
	return (rand() % 2) == 0;
}

/* Used too know if a point is enabled or not. */
bool enabled[NPOINTS];
pthread_rwlock_t enabled_lock;


/* Calls all the *enabled* points all the time. */
unsigned long long checking_caller_count = 0;
void *checking_caller(void *unused)
{
	int i = 0;
	int failed;

	while (!stop_threads) {
		pthread_rwlock_rdlock(&enabled_lock);
		if (enabled[i]) {
			failed = fiu_fail(point_name[i]) != 0;
			pthread_rwlock_unlock(&enabled_lock);

			if (!failed) {
				printf("ERROR: fp %d did not fail\n", i);
				assert(false);
			}
		} else {
			pthread_rwlock_unlock(&enabled_lock);
		}

		i = (i + 1) % NPOINTS;

		if (i == 0)
			checking_caller_count++;
	}

	checking_caller_count = checking_caller_count * NPOINTS + i;

	return NULL;
}

/* Enable and disable points all the time. */
unsigned long long enabler_count = 0;
void *enabler(void *unused)
{
	int i = 0;

	while (!stop_threads) {
		if (rand_bool()) {
			pthread_rwlock_wrlock(&enabled_lock);
			if (enabled[i]) {
				assert(fiu_disable(point_name[i]) == 0);
				enabled[i] = false;
			} else {
				assert(fiu_enable(point_name[i], 1, NULL, 0)
						== 0);
				enabled[i] = true;
			}
			pthread_rwlock_unlock(&enabled_lock);
		}

		i = (i + 1) % NPOINTS;

		if (i == 0)
			enabler_count++;
	}

	enabler_count = enabler_count * NPOINTS + i;

	return NULL;
}

void disable_all()
{
	int i = 0;

	pthread_rwlock_wrlock(&enabled_lock);
	for (i = 0; i < NPOINTS; i++) {
		/* Note this could fail as we don't check if they're active or
		 * not, but here we don't care. */
		fiu_disable(point_name[i]);
	}
	pthread_rwlock_unlock(&enabled_lock);
}

int main(void)
{
	int i;

	pthread_t t1, t2, t3;

	fiu_init(0);

	for (i = 0; i < NPOINTS; i++)
		make_point_name(point_name[i], i);

	pthread_rwlock_init(&enabled_lock, NULL);

	pthread_create(&t1, NULL, no_check_caller, NULL);
	pthread_create(&t2, NULL, checking_caller, NULL);
	pthread_create(&t3, NULL, enabler, NULL);

	sleep(TEST_TIME);

	stop_threads = 1;

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);

	disable_all();

	pthread_rwlock_destroy(&enabled_lock);

	printf("parallel nc: %-8llu  c: %-8llu  e: %-8llu  t: %llu\n",
			no_check_caller_count, checking_caller_count,
			enabler_count,
			no_check_caller_count + checking_caller_count
				+ enabler_count);

	return 0;
}

