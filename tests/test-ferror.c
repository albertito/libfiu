/* Test the handling of FILE * errors. */

#include <fiu.h>
#include <fiu-control.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


int test(const char *prefix) {
	FILE *fp = fopen("/dev/zero", "r");

	unsigned char buf[1024];
	ssize_t r;

	fiu_enable("posix/stdio/rw/fread", 1, (void *) EIO, 0);

	r = fread(buf, 1, 1024, fp);

	fiu_disable("posix/stdio/rw/fread");

	if (r != 0) {
		printf("%s: fread() succeeded, should have failed\n", prefix);
		return -1;
	}

	if (errno != EIO) {
		printf("%s: errno not set appropriately: ", prefix);
		printf("errno = %d / %s, expected EIO\n", errno, strerror(errno));
		return -1;
	}

	if (ferror(fp) == 0) {
		printf("%s: ferror() said there was no failure, but there was\n",
				prefix);
		return -1;
	}

	clearerr(fp);

	if (ferror(fp) != 0) {
		printf("%s: clearerr(), ferror() said there were failures\n",
				prefix);
		return -1;
	}

	fclose(fp);
	// Unfortunately we can't easily test after fclose() has been called,
	// because it's impossible to distinguish between a libfiu failure
	// from a libc error (which would appear due to an operation on a
	// closed file). To make things worse, some versions of glibc make
	// ferror() return an error on closed files, but others work.

	return 0;
}

int main(void) {
	// Run the test many times, to stress structure reuse a bit. This is
	// not as thorough but does exercise some bugs we've had, such as
	// forgetting to decrement the recursion counter.
	char prefix[8];
	for (int i = 0; i < 200; i++) {
		snprintf(prefix, 8, "%2d", i);
		test(prefix);
	}
}
