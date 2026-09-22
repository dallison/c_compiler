// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <initializer_list>
#include <type_traits>

std::false_type Or(std::initializer_list<std::false_type>);
std::true_type Or(std::initializer_list<bool>);

template <typename T>
constexpr bool Should() {
  return std::is_class<T>::value && std::is_empty<T>::value &&
         !std::is_final<T>::value;
}

template <typename... Ts>
constexpr bool Any() {
  return decltype(Or({std::integral_constant<bool, Should<Ts>()>()...})){};
}

struct Empty {};
struct FinalEmpty final {};
struct NonEmpty {
  int x;
};

int main() {
  using FromFalse = decltype(Or({std::false_type{}}));
  using FromTrue = decltype(Or({std::true_type{}}));
  using FromMix = decltype(Or({std::true_type{}, std::false_type{}}));
  using FromLiteralTrue = decltype(Or({std::integral_constant<bool, true>{}}));

  if (!std::is_same<FromFalse, std::false_type>::value) {
    return 1;
  }
  if (!std::is_same<FromTrue, std::true_type>::value) {
    return 2;
  }
  if (!std::is_same<FromMix, std::true_type>::value) {
    return 3;
  }
  if (!std::is_same<FromLiteralTrue, std::true_type>::value) {
    return 4;
  }
  if (Any<int, char>()) {
    return 5;
  }
  if (Any<NonEmpty, FinalEmpty>()) {
    return 6;
  }
  if (!Any<int, Empty>()) {
    return 7;
  }
  if (!Any<Empty>()) {
    return 8;
  }
  return 0;
}
