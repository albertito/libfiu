
/* Since our implementation relies on nonstandard functions and headers, we
 * provide a dummy one below for compatibility purposes. The dummy version can
 * be selected at build time using -DDUMMY_BACKTRACE. */
#ifndef DUMMY_BACKTRACE

/* This is needed for some of the functions below. */
#define _GNU_SOURCE

#include <stdlib.h>	/* NULL */
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

/* Ugly but useful conversion from function pointer to void *.
 * This is not guaranteed by the standard, but has to work on all platforms
 * where we support backtrace(), because that function assumes it so. */
static void *fp_to_voidp(void (*funcp)())
{
	unsigned char **p;
	p = (unsigned char **) &funcp;
	return *p;
}

int backtrace_works(void (*caller)())
{
	/* We remember the result so we don't have to compute it over an over
	 * again, we know it doesn't change. */
	static int works = -1;

	void *start = NULL;
	void *end = NULL;
	void *bt_buffer[100];
	void *pc;
	int nptrs, i;

	/* Return the result if we know it. */
	if (works >= 0)
		return works;

	nptrs = get_backtrace(bt_buffer, 100);
	if (nptrs <= 0) {
		works = 0;
		return works;
	}

	/* We will detect if it works by looking for the caller in the
	 * backtrace. */
	start = get_func_start(fp_to_voidp(caller));
	end = get_func_end(fp_to_voidp(caller));

	if (start == NULL && end == NULL) {
		works = 0;
		return works;
	}

	for (i = 0; i < nptrs; i++) {
		pc = bt_buffer[i];

		/* On some platforms, we have everything except
		 * get_func_end(), and that's ok. */
		if (end) {
			if (pc >= start && pc <= end) {
				works = 1;
				return works;
			}
		} else {
			if (get_func_start(pc) == start) {
				works = 1;
				return works;
			}
		}
	}

	works = 0;
	return works;
}
