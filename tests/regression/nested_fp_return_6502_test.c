#include <math.h>

static double scale(double value, int power) {
  if (power > 0) {
    while (power-- > 0) value += value;
  } else if (power < 0) {
    while (power++ < 0) value *= 0.5;
  }
  return value;
}

static double helper(double x) {
  return scale(x, 2);
}

int main(void) {
  double e = helper(1.0);
  if (e != 4.0) return 40;
  return 0;
}
