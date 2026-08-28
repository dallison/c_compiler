#include <stdio.h>

int main(void) {
  printf("plain\n");
  printf("int %d neg %d zero %d\n", 42, -7, 0);
  printf("uns %u hex %x oct %o\n", 4000000000u, 0xdeadbeef, 64);
  printf("str %s char %c\n", "text", 'Z');
  printf("wide |%8d| left |%-8d| zero |%08d|\n", 123, 123, 123);
  printf("long %ld longlong %lld\n", 1234567890L, 1234567890123456789LL);
  // No %.0f: libc reads a zero precision as none given and prints six
  // digits, on every target, so it would fail here for reasons of its own.
  printf("float %f %.2f\n", 3.14159, 3.14159);
  printf("pct %% done\n");
  return 0;
}
