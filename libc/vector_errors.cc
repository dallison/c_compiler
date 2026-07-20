#include <vector>

namespace std {
namespace __vector_detail {

void __throw_out_of_range() {
  throw out_of_range("vector::at");
}

void __throw_length_error() {
  throw length_error("vector::reserve");
}

}  // namespace __vector_detail
}  // namespace std
