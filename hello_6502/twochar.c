#include <stdio.h>

int main(void) {
  fputc('H', stdout);
  fputc('i', stdout);
  return fflush(stdout) == 0 ? 0 : 1;
}
