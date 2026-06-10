/* Regression test for RISC-V call-argument register shuffling.
   printf's internal GetNextArgument helper is called with 10 arguments, so
   setting up the argument registers requires resolving a parallel copy that
   includes a register swap (a1<->a2) and a chained move (the conversion
   command must reach a0 from a register another argument overwrites).  A naive
   sequential move sequence clobbered those values and produced zero/garbage
   output.  Exercising several conversions catches the regression on any
   backend while staying width-independent for the shared .expected file. */
#include <stdio.h>

int main(void) {
  printf("%d\n", 42);
  printf("%s %d\n", "hi", 7);
  printf("%d %d %d %d\n", 1, 2, 3, 4);
  printf("%c%c %x\n", 'A', 'B', 255);
  printf("%d %s %d\n", -5, "mid", 99);
  return 0;
}
