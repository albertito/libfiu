// A library that uses some of the same function names as libfiu.
// We use this to test function name collissions.

int called_wtable_get = 0;

void wtable_get(void) { called_wtable_get++; }
