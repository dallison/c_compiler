#include "test_framework.h"

#include <stdlib.h>

int TestAbs(void) {
  int failures = 0;

  CHECK_EQ(abs(-7), 7, failures);
  CHECK_EQ(abs(0), 0, failures);
  CHECK_EQ(abs(11), 11, failures);
  CHECK_EQ(labs(-9L), 9L, failures);
  CHECK_EQ(llabs(-13LL), 13LL, failures);
  return failures;
}

#if 0
// Enable once setjmp/longjmp restore the interpreter stack correctly.
#include <setjmp.h>

int TestSetjmp(void) {
  jmp_buf env;
  int failures = 0;
  int val = setjmp(env);
  if (val == 0) {
    longjmp(env, 42);
  }
  CHECK_EQ(val, 42, failures);
  return failures;
}
#endif
