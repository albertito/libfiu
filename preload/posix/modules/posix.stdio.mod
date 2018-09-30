
# Posix stdio.h I/O

include: <stdio.h>
include: <errno.h>
include: <stdarg.h>

fiu name base: posix/stdio/oc/

FILE *fopen(const char *pathname, const char *mode);
	on error: NULL
	valid errnos: EACCES EINTR EISDIR ELOOP EMFILE ENAMETOOLONG ENFILE ENOENT ENOTDIR ENOSPC ENXIO EOVERFLOW EROFS EINVAL ENOMEM ETXTBSY
	variants: off64_t

FILE *freopen(const char *pathname, const char *mode, FILE *stream);
	on error: NULL
	valid errnos: EACCES EBADF EINTR EISDIR ELOOP EMFILE ENAMETOOLONG ENFILE ENOENT ENOTDIR ENOSPC ENXIO EOVERFLOW EROFS EBADF EINVAL ENOMEM ENXIO ETXTBSY
	variants: off64_t

# This one needs to be further instrumented.
# int fclose(FILE *stream);
# 	on error: EOF
# 	valid errnos: EAGAIN EBADF EFBIG EFBIG EINTR EIO ENOMEM ENOSPC EPIPE ENXIO

FILE *fdopen(int fd, const char *mode);
	on error: NULL
	valid errnos: EMFILE EBADF EINVAL EMFILE ENOMEM

FILE *fmemopen(void *restrict buf, size_t size, const char *restrict mode);
	on error: NULL
	valid errnos: EMFILE EINVAL ENOMEM

FILE *open_memstream(char **bufp, size_t *sizep);
	on error: NULL
	valid errnos: EMFILE EINVAL ENOMEM

FILE *popen(const char *command, const char *mode);
	on error: NULL
	valid errnos: EMFILE EINVAL ENOMEM EAGAIN ENFILE

int pclose(FILE *stream);
	on error: -1
	valid errnos: ECHILD


fiu name base: posix/stdio/tmp/

FILE *tmpfile(void);
	on error: NULL
	valid errnos: EINTR EMFILE ENFILE ENOSPC EOVERFLOW ENOMEM
	variants: off64_t

char *tmpnam(char *s);
	on error: NULL

char *tempnam(const char *dir, const char *pfx);
	on error: NULL
	valid errnos: ENOMEM

fiu name base: posix/stdio/rw/

size_t fread(void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream);
	on error: 0
	valid errnos: EAGAIN EBADF EINTR EIO EOVERFLOW ENOMEM ENXIO
	ferror: stream

size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict stream);
	on error: 0
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOSPC EPIPE ENOMEM ENXIO
	ferror: stream

fiu name base: posix/stdio/seek/

int fgetpos(FILE *restrict stream, fpos_t *restrict pos);
	on error: -1
	valid errnos: EBADF EOVERFLOW ESPIPE

long ftell(FILE *stream);
	on error: -1
	valid errnos: EBADF EOVERFLOW ESPIPE

off_t ftello(FILE *stream);
	on error: -1
	valid errnos: EBADF EOVERFLOW ESPIPE
	variants: off64_t

int fseek(FILE *stream, long int offset, int whence);
	on error: -1
	valid errnos: EAGAIN EBADF EFBIG EINTR EINVAL EIO ENOSPC EOVERFLOW EPIPE ENXIO
	ferror: stream

int fseeko(FILE *stream, off_t offset, int whence);
	on error: -1
	valid errnos: EAGAIN EBADF EFBIG EINTR EINVAL EIO ENOSPC EOVERFLOW EPIPE ENXIO
	ferror: stream
	variants: off64_t

int fsetpos(FILE *stream, const fpos_t *pos);
	on error: -1
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOSPC EOVERFLOW EPIPE ENXIO
	ferror: stream
	variants: off64_t

# void rewind(FILE *stream);
#	valid errnos: EAGAIN EBADF EFBIG EINTR EINVAL EIO ENOSPC EOVERFLOW EPIPE ENXIO
#	ferror: stream


fiu name base: posix/stdio/gp/

int fgetc(FILE *stream);
	on error: EOF
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW
	ferror: stream

char *fgets(char *restrict s, int n, FILE *restrict stream);
	on error: NULL
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW
	ferror: stream

int getc(FILE *stream);
	on error: EOF
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW
	ferror: stream

int getchar(void);
	on error: EOF
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW
	ferror: stdin

char *gets(char *s);
	on error: NULL
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW
	ferror: stdin

int fputc(int c, FILE *stream);
	on error: EOF
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC ENXIO EPIPE
	ferror: stream

int fputs(const char *restrict s, FILE *restrict stream);
	on error: EOF
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC ENXIO EPIPE
	ferror: stream

#ifndef putc
int putc(int c, FILE *stream);
	on error: EOF
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC ENXIO EPIPE
	ferror: stream
#endif

int putchar(int c);
	on error: EOF
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC ENXIO EPIPE
	ferror: stdout

int puts(const char *s);
	on error: EOF
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC ENXIO EPIPE
	ferror: stdout

int ungetc(int c, FILE *stream);
	on error: EOF
	ferror: stream

ssize_t getdelim(char **restrict lineptr, size_t *restrict n, int delimiter, FILE *restrict stream);
	on error: -1
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW INVAL
	ferror: stream

ssize_t getline(char **restrict lineptr, size_t *restrict n, FILE *restrict stream);
	on error: -1
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW INVAL
	ferror: stream


fiu name base: posix/stdio/sp/

# Variants with a variable number of arguments need a custom definition.

# int fprintf(FILE *restrict stream, const char *restrict format, ...);
# int printf(const char *restrict format, ...);
# int dprintf(int fildes, const char *restrict format, ...);

int vfprintf(FILE *restrict stream, const char *restrict format, va_list ap);
	on error: -1
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC ENXIO EPIPE EILSEQ EOVERFLOW
	ferror: stream

int vprintf(const char *restrict format, va_list ap);
	on error: -1
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC ENXIO EPIPE EILSEQ EOVERFLOW
	ferror: stdout

int vdprintf(int fildes, const char *restrict format, va_list ap);
	on error: -1
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC ENXIO EPIPE EILSEQ EOVERFLOW

# int fscanf(FILE *restrict stream, const char *restrict format, ...);
# int scanf(const char *restrict format, ...);

int vfscanf(FILE *restrict stream, const char *restrict format, va_list arg);
	on error: EOF
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW EILSEQ EINVAL
	ferror: stream

int vscanf(const char *restrict format, va_list arg);
	on error: EOF
	valid errnos: EAGAIN EBADF EINTR EIO ENOMEM ENXIO EOVERFLOW EILSEQ
	ferror: stdin

fiu name base: posix/stdio/

# Other functions not worth categorizing

int remove(const char *filename);
	on error: -1
	valid errnos: EACCES EBUSY EEXIST ENOTEMPTY EINVAL EIO ELOOP ENAMETOOLONG ENOENT ENOTDIR EPERM EROFS ETXTBSY

int setvbuf(FILE *restrict stream, char *restrict buf, int type, size_t size);
	on error: EOF
	valid errnos: EBADF

int ftrylockfile(FILE *file);
	on error: 1

int fflush(FILE *stream);
	on error: EOF
	valid errnos: EAGAIN EBADF EFBIG EINTR EIO ENOMEM ENOSPC EPIPE ENXIO
	ferror: stream


