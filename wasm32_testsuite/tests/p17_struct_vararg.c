#include <stdarg.h>

struct pair { int a; int b; };
struct small { char x[3]; };
struct wide { long long lo; long long hi; };

static int sum_pairs(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int total = 0;
  for (int i = 0; i < n; i++) {
    struct pair value = va_arg(ap, struct pair);
    total += value.a + value.b;
  }
  va_end(ap);
  return total;
}

static int mix(int n, ...) {
  va_list ap;
  va_start(ap, n);
  struct pair first = va_arg(ap, struct pair);
  int mid = va_arg(ap, int);
  struct small second = va_arg(ap, struct small);
  struct wide third = va_arg(ap, struct wide);
  va_end(ap);
  return first.a + first.b + mid + second.x[0] + second.x[2] +
         (int)(third.lo + third.hi) + n;
}

int main(void) {
  struct pair one = {1, 2};
  struct pair two = {3, 4};
  struct small s = {{5, 0, 6}};
  struct wide w = {7, 8};
  return sum_pairs(2, one, two) + mix(9, one, 10, s, w);
}
