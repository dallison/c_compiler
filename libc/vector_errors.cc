#include <__exception_support>
#include <vector>
#ifdef __cpp_exceptions
#include <stdexcept>
#endif

namespace std {
namespace __vector_detail {

void __throw_out_of_range() {
  __DAVECC_THROW(out_of_range("vector::at"));
}

void __throw_length_error() {
  __DAVECC_THROW(length_error("vector::reserve"));
}

}  // namespace __vector_detail
}  // namespace std
