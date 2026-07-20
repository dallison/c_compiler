#include "string_conversion_helpers.h"

#include <cerrno>
#include <cstdlib>

namespace std {

unsigned long stoul(const string& input, size_t* pos, int base) {
  char* end = nullptr;
  errno = 0;
  unsigned long value = strtoul(input.c_str(), &end, base);
  __string_detail::CheckConversion(input, end, "stoul");
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return value;
}

}  // namespace std
