
include: <unistd.h>
include: <errno.h>
include: <sys/mman.h>

fiu name base: posix/mm/


v: #ifdef _POSIX_MAPPED_FILES

void *mmap(void *addr, size_t length, int prot, int flags, int fd, \
		off_t offset);
	on error: MAP_FAILED
	valid errnos: EACCES EAGAIN EBADF EINVAL ENFILE ENODEV ENOMEM EPERM \
		ETXTBSY
	variants: off64_t

int munmap(void *addr, size_t length);
	on error: 0
	valid errnos: EACCES EAGAIN EBADF EINVAL ENFILE ENODEV ENOMEM EPERM \
		ETXTBSY


int msync(void *addr, size_t length, int flags);
	on error: -1
	valid errnos: EBUSY EINVAL ENOMEM

# glibc's mprotect() does not use const in the first argument, as the standard
# says it should
v: #ifdef __GLIBC__
int mprotect(void *addr, size_t len, int prot);
	on error: -1
	valid errnos: EACCES EINVAL ENOMEM
v: #else
int mprotect(const void *addr, size_t len, int prot);
	on error: -1
	valid errnos: EACCES EINVAL ENOMEM
v: #endif

int madvise(void *addr, size_t length, int advice);
	on error: -1
	valid errnos: EAGAIN EBADF EINVAL EIO ENOMEM

v: #else
v:   #warning "no mmap() (and friends) wrappers available"
v: #endif


v: #ifdef _POSIX_MEMLOCK_RANGE

int mlock(const void *addr, size_t len);
	on error: -1
	valid errnos: ENOMEM EPERM EAGAIN EINVAL

int munlock(const void *addr, size_t len);
	on error: -1
	valid errnos: ENOMEM EPERM EAGAIN EINVAL

v: #else
v:   #warning "no mlock()/munlock() wrappers available"
v: #endif


v: #ifdef _POSIX_MEMLOCK

int mlockall(int flags);
	on error: -1
	valid errnos: ENOMEM EPERM EINVAL

int munlockall(void);
	on error: -1
	valid errnos: ENOMEM EPERM

v: #else
v:   #warning "no mlockall()/munlockall() wrappers available"
v: #endif

