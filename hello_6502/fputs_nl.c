#include <stdio.h>

int main(void) {
  return fputs("Hi\n", stdout) >= 0 ? 0 : 1;
}
