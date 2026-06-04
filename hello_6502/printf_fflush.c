#include <stdio.h>

int main(void) {
  printf("Hi");
  return fflush(stdout) == 0 ? 0 : 1;
}
