#include "string_conversion_helpers.h"

#include <__itoa.h>

namespace std {

string to_string(int value) {
  char buffer[__DAVECC_ITOA_CAPACITY(int)];
  size_t length = __itoa_int(buffer, value, 10, 0);
  return string(buffer, length);
}

}  // namespace std
