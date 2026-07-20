#include "string_conversion_helpers.h"

#include <cerrno>
#include <cstdlib>

namespace std {

float stof(const string& input, size_t* pos) {
  char* end = nullptr;
  errno = 0;
  float value = strtof(input.c_str(), &end);
  __string_detail::CheckConversion(input, end, "stof");
  if (pos != nullptr) {
    *pos = static_cast<size_t>(end - input.c_str());
  }
  return value;
}

}  // namespace std
