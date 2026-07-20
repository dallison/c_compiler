#include <map>

namespace std {
namespace __map_detail {

void __throw_out_of_range() {
  throw out_of_range("map::at");
}

}  // namespace __map_detail
}  // namespace std
