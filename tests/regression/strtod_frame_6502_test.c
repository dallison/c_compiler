#include <stdlib.h>

// strtod finishes in __packIEEE754 (assembly), which must pop its own
// arguments. If it does not, strtod hands back a stack 12 bytes short and
// restores its caller's frame pointer and saved registers from the wrong
// slots.

volatile unsigned probe_addr;

__attribute__((noinline)) static unsigned probe(void) {
  volatile char c = 0;
  probe_addr = (unsigned)&c;
  return probe_addr;
}

__attribute__((noinline)) static int parse_double(const char* s, double lo,
                                                  double hi) {
  volatile unsigned guard = 0x1234;
  double d = strtod(s, NULL);
  if (guard != 0x1234) return 1;
  return d > lo && d < hi ? 0 : 2;
}

__attribute__((noinline)) static int parse_float(const char* s, float lo,
                                                 float hi) {
  volatile unsigned guard = 0x5678;
  float f = strtof(s, NULL);
  if (guard != 0x5678) return 3;
  return f > lo && f < hi ? 0 : 4;
}

int main(void) {
  unsigned before = probe();
  for (int i = 0; i < 3; i++) {
    int r = parse_double("-1.25e-3", -0.0013, -0.0012);
    if (r) return r;
    r = parse_float("2.5", 2.49f, 2.51f);
    if (r) return r;
  }
  return probe() == before ? 0 : 5;
}
