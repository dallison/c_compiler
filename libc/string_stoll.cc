#include "string_conversion_helpers.h"

#include <cerrno>
#include <cstdlib>

namespace std {

long long stoll(const string& input, size_t* pos, int base) {
  char* end = nullptr;
  errno = 0;
  long long value = strtoll(input.c_str(), &end, base);
  __string_detail::CheckConversion(input, end, "stoll");
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return value;
}

}  // namespace std
