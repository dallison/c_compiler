#include "string_conversion_helpers.h"

namespace std {
namespace __string_detail {

string FromNumericBuffer(const char* buffer, size_t capacity, int length) {
  if (length < 0) {
    return string();
  }
  size_t size = static_cast<size_t>(length);
  if (size >= capacity) {
    size = capacity - 1;
  }
  return string(buffer, size);
}

}  // namespace __string_detail
}  // namespace std
