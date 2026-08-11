#include "random_host.h"

#include <stdint.h>
#include <string.h>

int main(void) {
  uint8_t first[32] = {0};
  uint8_t second[32] = {0};
  uint8_t zero[32] = {0};

  if (DaveHostRandomBytes(NULL, 0) != 0) return 1;
  if (DaveHostRandomBytes(NULL, 1) == 0) return 2;
  if (DaveHostRandomBytes(first, sizeof(first)) != 0) return 3;
  if (DaveHostRandomBytes(second, sizeof(second)) != 0) return 4;
  if (memcmp(first, zero, sizeof(first)) == 0) return 5;
  if (memcmp(first, second, sizeof(first)) == 0) return 6;
  return 0;
}
