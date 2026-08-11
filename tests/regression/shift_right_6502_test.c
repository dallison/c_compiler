#include <stdint.h>

int main(void) {
  uint32_t v = 0x007fffffu;
  int n = 1;
  return (int)((v >> n) & 0xffu);
}
