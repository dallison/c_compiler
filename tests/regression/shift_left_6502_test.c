#include <stdint.h>

int main(void) {
  uint32_t v = 1u;
  int n = 22;
  return (int)((v << n) & 0xffu);
}
