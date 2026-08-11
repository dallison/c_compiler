#include <math.h>

int main(void) {
  int e;
  double m = frexp(8.0, &e);
  if (m != 0.5) {
    return 10;
  }
  if (e != 4) {
    return 20;
  }
  return 0;
}
