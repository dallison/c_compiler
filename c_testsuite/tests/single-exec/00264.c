#include <stdio.h>
#include <string.h>

// __FILE__ must spell the whole name however long it is (absl records it for
// every flag, and builds pass absolute paths).
#define FILE_THROUGH_MACRO() __FILE__

int main(void) {
  const char* file = __FILE__;
  size_t length = strlen(file);
  if (length < 7 || strcmp(file + length - 7, "00264.c") != 0) return 1;
#line 100 "a/rather/long/directory/name/for/the/line/directive.c"
  if (strcmp(__FILE__, "a/rather/long/directory/name/for/the/line/directive.c") != 0) return 2;
  if (strcmp(FILE_THROUGH_MACRO(), __FILE__) != 0) return 3;
  if (sizeof(__FILE__) != sizeof("a/rather/long/directory/name/for/the/line/directive.c")) return 4;
  printf("ok\n");
  return 0;
}
