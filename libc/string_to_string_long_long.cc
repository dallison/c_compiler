#include "string_conversion_helpers.h"

#include <cstdio>

namespace std {

string to_string(long long value) {
  char buffer[32];
  int length = snprintf(buffer, sizeof(buffer), "%lld", value);
  return __string_detail::FromNumericBuffer(buffer, sizeof(buffer), length);
}

}  // namespace std
