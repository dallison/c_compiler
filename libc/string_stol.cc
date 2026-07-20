#include "string_conversion_helpers.h"

#include <cerrno>
#include <cstdlib>

namespace std {

long stol(const string& input, size_t* pos, int base) {
  char* end = nullptr;
  errno = 0;
  long value = strtol(input.c_str(), &end, base);
  __string_detail::CheckConversion(input, end, "stol");
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return value;
}

}  // namespace std
