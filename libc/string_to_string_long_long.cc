#include "string_conversion_helpers.h"

#include <__itoa.h>

namespace std {

string to_string(long long value) {
  char buffer[__DAVECC_ITOA_CAPACITY(long long)];
  size_t length = __itoa_longlong(buffer, value, 10, 0);
  return string(buffer, length);
}

}  // namespace std
