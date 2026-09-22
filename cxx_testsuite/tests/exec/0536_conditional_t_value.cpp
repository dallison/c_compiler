// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class T>
struct ConditionalValue
    : std::integral_constant<
          bool, std::conditional_t<std::is_reference_v<T>, std::false_type,
                                   std::true_type>::value> {};

int main() {
  if (!std::conditional_t<false, std::false_type, std::true_type>::value) {
    return 1;
  }
  if (std::conditional_t<true, std::false_type, std::true_type>::value) {
    return 2;
  }
  if (!ConditionalValue<int>::value) {
    return 3;
  }
  if (ConditionalValue<int&>::value) {
    return 4;
  }
  return 0;
}
