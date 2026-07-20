#include "string_conversion_helpers.h"

#include <cstdio>

namespace std {

string to_string(double value) {
  char buffer[384];
  int length = snprintf(buffer, sizeof(buffer), "%f", value);
  return __string_detail::FromNumericBuffer(buffer, sizeof(buffer), length);
}

}  // namespace std
