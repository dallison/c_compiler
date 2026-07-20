#include "string_conversion_helpers.h"

#include <cstdio>

namespace std {

string to_string(int value) {
  char buffer[16];
  int length = snprintf(buffer, sizeof(buffer), "%d", value);
  return __string_detail::FromNumericBuffer(buffer, sizeof(buffer), length);
}

}  // namespace std
