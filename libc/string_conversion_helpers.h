#ifndef __davecc_string_conversion_helpers_h
#define __davecc_string_conversion_helpers_h

#include <cstddef>
#include <string>

namespace std {
namespace __string_detail {

string FromNumericBuffer(const char* buffer, size_t capacity, int length);
void CheckConversion(const string& input, const char* end,
                     const char* function_name);

}  // namespace __string_detail
}  // namespace std

#endif
