#include "string_conversion_helpers.h"

#include <cerrno>
#include <cstdlib>

namespace std {

long double stold(const string& input, size_t* pos) {
  char* end = nullptr;
  errno = 0;
  long double value = strtold(input.c_str(), &end);
  __string_detail::CheckConversion(input, end, "stold");
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return value;
}

}  // namespace std
