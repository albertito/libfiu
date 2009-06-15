
include: <fcntl.h>
include: <errno.h>

fiu name base: linux/io/

int sync_file_range(int fd, off_t offset, off_t nbytes, \
		unsigned int flags);
	on error: -1
	valid errnos: EBADF EINVAL EIO ENOMEM ENOSPC


