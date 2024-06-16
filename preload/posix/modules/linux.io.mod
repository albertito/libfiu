
include: <fcntl.h>
include: <errno.h>

fiu name base: linux/io/

# sync_file_range() is linux-only, and depends on _FILE_OFFSET_BITS==64.
v: #if defined __linux__ && _FILE_OFFSET_BITS == 64

int sync_file_range(int fd, off_t offset, off_t nbytes, \
		unsigned int flags);
	on error: -1
	valid errnos: EBADF EINVAL EIO ENOMEM ENOSPC

v: #endif

