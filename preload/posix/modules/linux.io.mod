
include: <fcntl.h>
include: <errno.h>

fiu name base: linux/io/

# sync_file_range() is linux-only
v: #ifdef __linux__

int sync_file_range(int fd, off64_t offset, off64_t nbytes, \
		unsigned int flags);
	on error: -1
	valid errnos: EBADF EINVAL EIO ENOMEM ENOSPC

v: #endif

