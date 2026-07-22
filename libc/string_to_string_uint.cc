#include "string_conversion_helpers.h"

#include <__itoa.h>

namespace std {

string to_string(unsigned int value) {
  char buffer[__DAVECC_ITOA_CAPACITY(unsigned int)];
  size_t length = __utoa_uint(buffer, value, 10, 0);
  return string(buffer, length);
}

}  // namespace std
