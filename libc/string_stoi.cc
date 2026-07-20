#include "string_conversion_helpers.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <stdexcept>

namespace std {

int stoi(const string& input, size_t* pos, int base) {
  char* end = nullptr;
  errno = 0;
  long value = strtol(input.c_str(), &end, base);
  __string_detail::CheckConversion(input, end, "stoi");
  if (value < INT_MIN || value > INT_MAX) {
    throw out_of_range("stoi");
  }
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return static_cast<int>(value);
}

}  // namespace std
