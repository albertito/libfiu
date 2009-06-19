
# Posix I/O

include: <sys/types.h>
include: <sys/uio.h>
include: <unistd.h>
include: <sys/socket.h>
include: <sys/stat.h>
include: <sys/select.h>
include: <poll.h>
include: <fcntl.h>
include: <errno.h>

fiu name base: posix/io/oc/

# open() has its own custom wrapper

int close(int fd);
	on error: -1
	valid errnos: EBADFD EINTR EIO


fiu name base: posix/io/sync/

int fsync(int fd);
	on error: -1
	valid errnos: EBADFD EIO EROFS EINVAL

int fdatasync(int fd);
	on error: -1
	valid errnos: EBADFD EIO EROFS EINVAL


fiu name base: posix/io/rw/

ssize_t read(int fd, void *buf, size_t count);
	on error: -1
	valid errnos: EBADFD EFAULT EINTR EINVAL EIO EISDIR EOVERFLOW

ssize_t pread(int fd, void *buf, size_t count, off_t offset);
	on error: -1
	valid errnos: EBADFD EFAULT EINTR EINVAL EIO EISDIR EOVERFLOW

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
	on error: -1
	valid errnos: EBADFD EFAULT EINTR EINVAL EIO EISDIR EOVERFLOW


ssize_t write(int fd, const void *buf, size_t count);
	on error: -1
	valid errnos: EBADFD EFAULT EFBIG EINTR EINVAL EIO ENOSPC

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
	on error: -1
	valid errnos: EBADFD EFAULT EFBIG EINTR EINVAL EIO ENOSPC \
		EOVERFLOW

ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
	on error: -1
	valid errnos: EBADFD EFAULT EFBIG EINTR EINVAL EIO ENOSPC


fiu name base: posix/io/dir/

include: <dirent.h>

DIR *opendir(const char *name);
	on error: NULL
	valid errnos: EACCES EBADF EMFILE ENFILE ENOENT ENOMEM ENOTDIR

DIR *fdopendir(int fd);
	on error: NULL
	valid errnos: EACCES EBADF EMFILE ENFILE ENOENT ENOMEM ENOTDIR

struct dirent *readdir(DIR *dirp);
	on error: NULL
	valid errnos: EBADF

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
	on error: 1

int closedir(DIR *dirp);
	on error: -1
	valid errnos: EBADF


fiu name base: posix/io/net/

int socket(int domain, int type, int protocol);
	on error: -1
	valid errnos: EAFNOSUPPORT EMFILE ENFILE EPROTONOSUPPORT EPROTOTYPE \
		EACCES ENOBUFS ENOMEM

int bind(int socket, const struct sockaddr *address, socklen_t address_len);
	on error: -1
	valid errnos: EADDRINUSE EADDRNOTAVAIL EAFNOSUPPORT EBADF EINVAL ENOTSOCK \
		EOPNOTSUPP EACCES EDESTADDRREQ EIO ELOOP ENAMETOOLONG ENOENT \
		ENOTDIR EROFS EACCES EINVAL EISCONN ELOOP ENAMETOOLONG \
		ENOBUFS

int listen(int socket, int backlog);
	on error: -1
	valid errnos: EBADF EDESTADDRREQ EINVAL ENOTSOCK EOPNOTSUPP EACCES EINVAL \
		ENOBUFS

int accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len);
	on error: -1
	valid errnos:  EAGAIN EBADF ECONNABORTED EINTR EINVAL EMFILE ENFILE \
		ENOTSOCK EOPNOTSUPP ENOBUFS ENOMEM EPROTO

int connect(int socket, const struct sockaddr *address, socklen_t address_len);
	on error: -1
	valid errnos:  EADDRNOTAVAIL EAFNOSUPPORT EALREADY EBADF ECONNREFUSED \
		EINPROGRESS EINTR EISCONN ENETUNREACH ENOTSOCK EPROTOTYPE \
		ETIMEDOUT EIO ELOOP ENAMETOOLONG ENOENT ENOTDIR EACCES \
		EADDRINUSE ECONNRESET EHOSTUNREACH EINVAL ELOOP ENAMETOOLONG \
		ENETDOWN ENOBUFS EOPNOTSUPP

ssize_t recv(int socket, void *buffer, size_t length, int flags);
	on error: -1
	valid errnos:  EAGAIN EBADF ECONNRESET EINTR EINVAL ENOTCONN ENOTSOCK \
		EOPNOTSUPP ETIMEDOUT EIO ENOBUFS ENOMEM

ssize_t recvfrom(int socket, void *restrict buffer, size_t length, int flags, struct sockaddr *restrict address, socklen_t *restrict address_len);
	on error: -1
	valid errnos:  EAGAIN EBADF ECONNRESET EINTR EINVAL ENOTCONN ENOTSOCK \
		EOPNOTSUPP ETIMEDOUT EIO ENOBUFS ENOMEM

ssize_t recvmsg(int socket, struct msghdr *message, int flags);
	on error: -1
	valid errnos:  EAGAIN EBADF ECONNRESET EINTR EINVAL EMSGSIZE ENOTCONN \
		ENOTSOCK EOPNOTSUPP ETIMEDOUT EIO ENOBUFS ENOMEM

ssize_t send(int socket, const void *buffer, size_t length, int flags);
	on error: -1
	valid errnos:  EAGAIN EBADF ECONNRESET EDESTADDRREQ EINTR EMSGSIZE \
		ENOTCONN ENOTSOCK EOPNOTSUPP EPIPE EACCES EIO ENETDOWN \
		ENETUNREACH ENOBUFS

ssize_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
	on error: -1
	valid errnos:  EAFNOSUPPORT EAGAIN EBADF ECONNRESET EINTR EMSGSIZE \
		ENOTCONN ENOTSOCK EOPNOTSUPP EPIPE EIO ELOOP ENAMETOOLONG \
		ENOENT ENOTDIR EACCES EDESTADDRREQ EHOSTUNREACH EINVAL EIO \
		EISCONN ENETDOWN ENETUNREACH ENOBUFS ENOMEM ELOOP \
		ENAMETOOLONG

ssize_t sendmsg(int socket, const struct msghdr *message, int flags);
	on error: -1
	valid errnos:  EAGAIN EAFNOSUPPORT EBADF ECONNRESET EINTR EINVAL EMSGSIZE \
		ENOTCONN ENOTSOCK EOPNOTSUPP EPIPE EIO ELOOP ENAMETOOLONG \
		ENOENT ENOTDIR EACCES EDESTADDRREQ EHOSTUNREACH EIO EISCONN \
		ENETDOWN ENETUNREACH ENOBUFS ENOMEM ELOOP ENAMETOOLONG

int shutdown(int socket, int how);
	on error: -1
	valid errnos:  EBADF EINVAL ENOTCONN ENOTSOCK ENOBUFS

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, \
		struct timeval *timeout);
	on error: -1
	valid errnos: EBADF EINTR EINVAL ENOMEM

int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, \
		const struct timespec *timeout, const sigset_t *sigmask);
	on error: -1
	valid errnos: EBADF EINTR EINVAL ENOMEM

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
	on error: -1
	valid errnos: EBADF EFAULT EINTR EINVAL ENOMEM


