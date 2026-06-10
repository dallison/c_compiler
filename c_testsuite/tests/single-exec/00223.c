/* Diagnostic / behavior attributes must parse and compile cleanly and not
   affect generated code.  The warnings they trigger go to stderr; this test
   only checks that the program still builds and runs. */
#include <stdio.h>

int twice(int x) __attribute__((warn_unused_result));
int twice(int x) { return x * 2; }

int legacy(void) __attribute__((deprecated("use twice")));
int legacy(void) { return 7; }

int my_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int my_printf(const char *fmt, ...) { return 0; }

int main(void) {
  int unused_local __attribute__((unused));
  int r = twice(legacy());   /* result consumed: no warn_unused_result */
  my_printf("%d\n", r);      /* well-formed format call */
  printf("%d\n", r);
  return 0;
}
