
/* Test creation and removal of wildcarded failure points while checking them
 * on a different thread.
 *
 * This test will have a thread enabling and disabling failure points like
 * point:number:1:*, point:number:2:*, ...
 *
 * Then it has two threads checking:
 * point:number:1:1, point:number:1:2, ...,
 * point:number:2:1, point:number:2:2, ...
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

/* Be careful with increasing these numbers, as the memory usage is directly
 * related to them. */
#define NHIGH 1000
#define NLOW  1000

/* Maximum number of a failure point's name */
#define MAX_FPNAME 32

/* How many seconds to run the test */
#define TEST_TIME 5

/* Signal for the threads to stop. */
bool stop_threads = false;

/* The name of each final point. */
char final_point_name[NHIGH][NLOW][MAX_FPNAME];

/* The name of each wildcarded point. */
char wildcard_point_name[NHIGH][MAX_FPNAME];

char *make_final_point_name(char *name, int high, int low)
{
	sprintf(name, "point/number/%d/%d", high, low);
	return name;
}

char *make_wildcard_point_name(char *name, int high)
{
	sprintf(name, "point/number/%d/*", high);
	return name;
}

/* Calls all the final points all the time. */
unsigned long long no_check_caller_count = 0;
void *no_check_caller(void *unused)
{
	int high, low;

	for (;;) {
		for (high = 0; high < NHIGH; high++) {
			for (low = 0; low < NLOW; low++) {
				fiu_fail(final_point_name[high][low]);
				no_check_caller_count++;
			}
			if (stop_threads)
				return NULL;
		}
	}

	return NULL;
}

bool rand_bool(void) {
	return (rand() % 2) == 0;
}

/* Used too know if a point is enabled or not. */
bool enabled[NHIGH];
pthread_rwlock_t enabled_lock;


/* Calls all the *enabled* points all the time. */
unsigned long long checking_caller_count = 0;
void *checking_caller(void *unused)
{
	int high, low;
	int failed;

	for (;;) {
		for (high = 0; high < NHIGH; high++) {
			pthread_rwlock_rdlock(&enabled_lock);
			if (!enabled[high]) {
				pthread_rwlock_unlock(&enabled_lock);
				continue;
			}

			for (low = 0; low < NLOW; low++) {
				failed = fiu_fail(
					final_point_name[high][low]) != 0;
				if (!failed) {
					printf("ERROR: %d:%d did not fail\n",
							high, low);
					assert(false);
				}
				checking_caller_count++;
			}
			pthread_rwlock_unlock(&enabled_lock);

			if (stop_threads)
				return NULL;
		}
	}

	return NULL;
}

/* Enable and disable wildcarded points all the time. */
unsigned long long enabler_count = 0;
void *enabler(void *unused)
{
	int high;

	for (;;) {
		for (high = 0; high < NHIGH; high++) {
			/* 50% chance of flipping this point. */
			if (rand_bool())
				continue;

			pthread_rwlock_wrlock(&enabled_lock);
			if (enabled[high]) {
				assert(fiu_disable(wildcard_point_name[high])
						== 0);
				enabled[high] = false;
			} else {
				assert(
					fiu_enable(
						wildcard_point_name[high],
						1, NULL, 0) == 0);
				enabled[high] = true;
			}
			pthread_rwlock_unlock(&enabled_lock);
			enabler_count++;

			if (stop_threads)
				return NULL;
		}
	}

	return NULL;
}

void disable_all()
{
	int high;

	pthread_rwlock_wrlock(&enabled_lock);
	for (high = 0; high < NHIGH; high++) {
		/* Note this could fail as we don't check if they're active or
		 * not, but here we don't care. */
		fiu_disable(wildcard_point_name[high]);
	}
	pthread_rwlock_unlock(&enabled_lock);
}

int main(void)
{
	int high, low;

	pthread_t t1, t2, t3;

	fiu_init(0);

	for (high = 0; high < NHIGH; high++) {
		make_wildcard_point_name(wildcard_point_name[high], high);

		for (low = 0; low < NLOW; low++) {
			make_final_point_name(final_point_name[high][low],
					high, low);
		}
	}

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

	printf("wildcard nc: %-8llu  c: %-8llu  e: %-8llu  t: %llu\n",
			no_check_caller_count, checking_caller_count,
			enabler_count,
			no_check_caller_count + checking_caller_count
				+ enabler_count);

	return 0;
}

