#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

// Some platforms the functions that have 64-bit (e.g. pread64, pwrite64) have
// an assembler alias declared for the base variant in the header; while other
// platforms don't do that and have the two functions declared separately.
//
// On the platforms that use assembler aliases, we can't redefine the 64-bit
// variants.
//
// See https://bugs.debian.org/1066938 for more details.
//
// To detect this, we define both functions here. If there is an assembler
// alias declared in the header, then compiling this will result in an error
// like "Error: symbol `pread64' is already defined".

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
	return 0;
}

ssize_t pread64(int fd, void *buf, size_t count, off_t offset) {
	return 0;
}
