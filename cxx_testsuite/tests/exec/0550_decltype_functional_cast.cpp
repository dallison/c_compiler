// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <initializer_list>
#include <type_traits>

std::false_type Or(std::initializer_list<std::false_type>);
std::true_type Or(std::initializer_list<bool>);

using Int = int;

constexpr int zero_from_decltype() {
  return decltype(0){};
}

int main() {
  if (zero_from_decltype() != 0) {
    return 1;
  }
  if (decltype(0)() != 0) {
    return 2;
  }
  if (int{} != 0) {
    return 3;
  }
  if (Int{} != 0) {
    return 4;
  }
  if (decltype(std::false_type{}){}) {
    return 5;
  }
  if (decltype(Or({std::false_type{}})){}) {
    return 6;
  }
  if (!decltype(Or({true})){}) {
    return 7;
  }
  return 0;
}
