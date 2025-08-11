
include: <unistd.h>
include: <errno.h>
include: <sys/mman.h>

fiu name base: posix/mm/

v: #if LIBFIU_CAN_DEFINE_64BIT_FUNCTIONS && defined _POSIX_MAPPED_FILES

void *mmap64(void *addr, size_t length, int prot, int flags, int fd, \
		off64_t offset);
	fiu name: posix/mm/mmap
	on error: MAP_FAILED
	valid errnos: EACCES EAGAIN EBADF EINVAL ENFILE ENODEV ENOMEM EPERM \
		ETXTBSY

v: #endif


