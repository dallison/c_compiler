#include <deque>

namespace std {
namespace __deque_detail {

void __throw_out_of_range() {
  throw out_of_range("deque::at");
}

}  // namespace __deque_detail
}  // namespace std
