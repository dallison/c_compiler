#include "string_conversion_helpers.h"

#include <__exception_support>
#include <cerrno>
#include <climits>
#include <cstdlib>
#ifdef __cpp_exceptions
#include <stdexcept>
#endif

namespace std {

int stoi(const string& input, size_t* pos, int base) {
  char* end = nullptr;
  errno = 0;
  long value = strtol(input.c_str(), &end, base);
  __string_detail::CheckConversion(input, end, "stoi");
  if (value < INT_MIN || value > INT_MAX) {
    __DAVECC_THROW(out_of_range("stoi"));
  }
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return static_cast<int>(value);
}

}  // namespace std
