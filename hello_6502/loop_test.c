#include <stdio.h>

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  const char* p = "Hi\n";
  while (*p) {
    putchar(*p++);
  }
  return 0;
}
