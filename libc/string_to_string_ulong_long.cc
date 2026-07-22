#include "string_conversion_helpers.h"

#include <__itoa.h>

namespace std {

string to_string(unsigned long long value) {
  char buffer[__DAVECC_ITOA_CAPACITY(unsigned long long)];
  size_t length = __utoa_ulonglong(buffer, value, 10, 0);
  return string(buffer, length);
}

}  // namespace std
