
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <fiu.h>
#include <fiu-control.h>

int func1()
{
	/*
	int nptrs;
	void *buffer[100];
	nptrs = backtrace(buffer, 100);
	backtrace_symbols_fd(buffer, nptrs, 1);
	*/

	return fiu_fail("fp-1") != 0;
}

int func2()
{
	return func1();
}


int main(void)
{
	int r;

	fiu_init(0);
	r = fiu_enable_stack_by_name("fp-1", 1, NULL, 0, "func2", -1);
	if (r != 0) {
		printf("NOTE: fiu_enable_stack_by_name() failed, "
				"skipping test\n");
		return 0;
	}

	assert(func1() == 0);
	assert(func2() == 1);

	fiu_disable("fp-1");

	assert(func1() == 0);
	assert(func2() == 0);

	return 0;
}

