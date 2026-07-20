#include "string_conversion_helpers.h"

#include <errno.h>
#include <stdexcept>

namespace std {
namespace __string_detail {

void CheckConversion(const string& input, const char* end,
                     const char* function_name) {
  if (end == input.c_str()) {
    throw invalid_argument(function_name);
  }
  if (errno == ERANGE) {
    throw out_of_range(function_name);
  }
}

}  // namespace __string_detail
}  // namespace std
