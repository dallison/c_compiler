#include <stdio.h>
#include <string.h>

int main(void) {
  fputs("Hi", stdout);
  return fflush(stdout) == 0 ? 0 : 1;
}
