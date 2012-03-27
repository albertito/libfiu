
/* Since our implementation relies on nonstandard functions and headers, we
 * provide a dummy one below for compatibility purposes. The dummy version can
 * be selected at build time using -DDUMMY_BACKTRACE. */
#ifndef DUMMY_BACKTRACE

/* This is needed for some of the functions below. */
#define _GNU_SOURCE

#include <execinfo.h>
#include <dlfcn.h>
#include <sys/procfs.h>
#include <link.h>


int get_backtrace(void *buffer, int size)
{
	return backtrace(buffer, size);
}

void *get_func_start(void *pc)
{
	int r;
	Dl_info info;

	r = dladdr(pc, &info);
	if (r == 0)
		return NULL;

	return info.dli_saddr;
}

void *get_func_end(void *func)
{
	int r;
	Dl_info dl_info;
	ElfW(Sym) *elf_info;

	r = dladdr1(func, &dl_info, (void **) &elf_info, RTLD_DL_SYMENT);
	if (r == 0)
		return NULL;
	if (elf_info == NULL)
		return NULL;
	if (dl_info.dli_saddr == NULL)
		return NULL;

	return ((unsigned char *) func) + elf_info->st_size;
}

void *get_func_addr(const char *func_name)
{
	return dlsym(RTLD_DEFAULT, func_name);
}

#else
/* Dummy versions */

#warning Using dummy versions of backtrace

#include <stddef.h>	/* for NULL */

int get_backtrace(void *buffer, int size)
{
	return 0;
}

void *get_func_end(void *pc)
{
	return NULL;
}

void *get_func_start(void *pc)
{
	return NULL;
}

void *get_func_addr(const char *func_name)
{
	return NULL;
}

#endif // DUMMY_BACKTRACE
