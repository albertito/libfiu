// A small "cat" utility that copies stdin to stdout.
// It does only one 4K read from stdin, and writes it to stdout in as few
// write()s as possible.
// This gives a controlled number of operations, which makes testing the
// random operations more robust.

#include <stdio.h>  // printf(), perror()
#include <unistd.h> // read(), write()

const size_t BUFSIZE = 4092;

int main(void)
{
	char buf[BUFSIZE];
	ssize_t r, w, pos;

	r = read(0, buf, BUFSIZE);
	if (r < 0) {
		perror("Read error in small-cat");
		return 1;
	}

	pos = 0;
	while (r > 0) {
		w = write(1, buf + pos, r);
		if (w <= 0) {
			perror("Write error in small-cat");
			return 2;
		}

		pos += w;
		r -= w;
	}

	return 0;
}
