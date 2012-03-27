
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <fiu.h>
#include <fiu-control.h>

void func1(int should_fail)
{
	int failed;

	/*
	int nptrs;
	void *buffer[100];
	nptrs = backtrace(buffer, 100);
	backtrace_symbols_fd(buffer, nptrs, 1);
	*/

	failed = fiu_fail("fp-1") != 0;
	assert(failed == should_fail);
}

void func2(int should_fail)
{
	func1(should_fail);
}


int main(void)
{
	fiu_init(0);
	fiu_enable_stack_by_name("fp-1", 1, NULL, 0, "func2", -1);

	func1(0);
	func2(1);


	fiu_disable("fp-1");

	func1(0);
	func2(0);

	return 0;
}

