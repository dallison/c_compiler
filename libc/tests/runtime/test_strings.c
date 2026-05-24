// String/memory behavioral tests for libc.  These are kept in-tree so we can
// enable them once x86_64 codegen can compile user calls to strcpy/strcmp/etc.
// Today, compiling this file crashes davecc at -O1; libc/strlen.c and friends
// still build as standalone translation units (see compile_all.sh).

#if 0

#include "test_framework.h"

#include <stddef.h>
#include <string.h>

int TestStrings(void) {
  char buf1[32];
  char buf2[32];
  int failures = 0;
  int cmp;

  strcpy(buf1, "hello");
  strcpy(buf2, "hello");
  cmp = strcmp(buf1, buf2);
  CHECK_EQ(cmp, 0, failures);

  strcat(buf1, " world");
  cmp = strcmp(buf1, "hello world");
  CHECK_EQ(cmp, 0, failures);
  CHECK_EQ((long)strlen(buf1), 11, failures);
  return failures;
}

int TestMemory(void) {
  char buf[16];
  int failures = 0;
  int cmp;

  memset(buf, 0, sizeof(buf));
  CHECK_EQ(buf[0], 0, failures);
  CHECK_EQ(buf[15], 0, failures);

  memset(buf, 'A', 8);
  CHECK_EQ(buf[0], 'A', failures);
  CHECK_EQ(buf[7], 'A', failures);
  CHECK_EQ(buf[8], 0, failures);

  memcpy(buf + 4, "test", 4);
  cmp = memcmp(buf + 4, "test", 4);
  CHECK_EQ(cmp, 0, failures);
  return failures;
}

#endif
