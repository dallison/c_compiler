/* #pragma message, #pragma GCC warning, and the _Pragma forms of each.

   These emit a note / warning to stderr (#pragma GCC error would abort
   compilation and is exercised separately), so this test only asserts that the
   pragmas are accepted and do not disturb parsing or codegen: it must still
   compile and print the expected value. */
#include <stdio.h>

#pragma message "compiling 00231"
#pragma message("parenthesized form")
#pragma GCC warning "a non-fatal pragma warning"
#pragma GCC warning("parenthesized pragma warning")

_Pragma("message \"message via _Pragma\"")
_Pragma("message(\"parenthesized message via _Pragma\")")
_Pragma("GCC warning \"warning via _Pragma\"")

int main(void) {
  _Pragma("message \"message from inside main\"")
  printf("%d\n", 42);
  return 0;
}
