// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class From, class To>
struct Helper : std::bool_constant<__davecc_is_convertible(From, To) != 0> {};

template <class From, class To>
struct Conv : Helper<From, To> {};

int main() {
  if (!std::is_convertible<int, int>::value) {
    return 1;
  }
  if (!std::is_convertible<int, long>::value) {
    return 2;
  }
  if (std::is_convertible<void*, int>::value) {
    return 3;
  }
  if (!Conv<int, int>::value) {
    return 4;
  }
  if (!std::is_base_of<Helper<int, long>, Conv<int, long>>::value) {
    return 5;
  }
  return Conv<int, long>::value ? 0 : 6;
}
