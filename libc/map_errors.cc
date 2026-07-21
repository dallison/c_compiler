#include <__exception_support>
#include <map>
#ifdef __cpp_exceptions
#include <stdexcept>
#endif

namespace std {
namespace __map_detail {

void __throw_out_of_range() {
  __DAVECC_THROW(out_of_range("map::at"));
}

}  // namespace __map_detail
}  // namespace std
