#include "string_conversion_helpers.h"

#include <__exception_support>
#include <errno.h>
#ifdef __cpp_exceptions
#include <stdexcept>
#endif

namespace std {
namespace __string_detail {

void CheckConversion(const string& input, const char* end,
                     const char* function_name) {
  if (end == input.c_str()) {
    __DAVECC_THROW(invalid_argument(function_name));
  }
  if (errno == ERANGE) {
    __DAVECC_THROW(out_of_range(function_name));
  }
}

}  // namespace __string_detail
}  // namespace std
