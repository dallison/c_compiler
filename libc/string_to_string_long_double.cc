#include "string_conversion_helpers.h"

#include <cstdio>

namespace std {

string to_string(long double value) {
  char buffer[384];
  int length = snprintf(buffer, sizeof(buffer), "%Lf", value);
  return __string_detail::FromNumericBuffer(buffer, sizeof(buffer), length);
}

}  // namespace std
