#include "string_conversion_helpers.h"

#include <cerrno>
#include <cstdlib>

namespace std {

unsigned long long stoull(const string& input, size_t* pos, int base) {
  char* end = nullptr;
  errno = 0;
  unsigned long long value = strtoull(input.c_str(), &end, base);
  __string_detail::CheckConversion(input, end, "stoull");
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return value;
}

}  // namespace std
