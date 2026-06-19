#include <stdio.h>

enum {
  folded_int = (2 + 3) * 4 - 5,
  folded_shift = (1 << 5) | 3,
  folded_cond = 0 ? 100 : folded_int + folded_shift,
};

static int static_folded = folded_cond + sizeof(int);
static double static_double = (1.5 + 2.5) * 2.0;

int main(void) {
  int values[folded_int == 15 ? 3 : -1];
  values[0] = folded_int;
  values[1] = folded_shift;
  values[2] = static_folded;

  switch (folded_cond) {
    case 50:
      printf("%d %d %d %.1f\n", values[0], values[1], values[2],
             static_double);
      return 0;
    default:
      return 1;
  }
}
