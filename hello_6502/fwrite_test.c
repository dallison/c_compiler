#include <stdio.h>

int main(void) {
  return fwrite("Hi", 1, 2, stdout) == 2 ? 0 : 1;
}
