#include <__exception_support>
#include <deque>
#ifdef __cpp_exceptions
#include <stdexcept>
#endif

namespace std {
namespace __deque_detail {

void __throw_out_of_range() {
  __DAVECC_THROW(out_of_range("deque::at"));
}

}  // namespace __deque_detail
}  // namespace std
