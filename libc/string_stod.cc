#include "string_conversion_helpers.h"

#include <cerrno>
#include <cstdlib>

namespace std {

double stod(const string& input, size_t* pos) {
  char* end = nullptr;
  errno = 0;
  double value = strtod(input.c_str(), &end);
  __string_detail::CheckConversion(input, end, "stod");
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return value;
}

}  // namespace std
