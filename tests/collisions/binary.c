// A binary that uses some of the same function names as libfiu.
// We use this to test function name collissions.

#include <stdio.h> // printf()
#include "libcoltest.h"

#define ASSERT_CALLED(NAME, N)                                                 \
	if (called_##NAME != N) {                                              \
		printf("Error: " #NAME "called %d != " #N "\n",                \
		       called_##NAME);                                         \
		return 1;                                                      \
	}

#define CHECK(NAME)                                                            \
	ASSERT_CALLED(NAME, 0)                                                 \
	NAME();                                                                \
	ASSERT_CALLED(NAME, 1)

int called_wtable_set = 0;

void wtable_set(void) { called_wtable_set++; }

int main(void)
{
	// Defined in libcoltest.
	CHECK(wtable_get)

	// Defined here.
	CHECK(wtable_set)

	return 0;
}
