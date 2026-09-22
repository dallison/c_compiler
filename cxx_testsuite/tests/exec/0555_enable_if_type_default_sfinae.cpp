// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>
#include <cstddef>

class OnlyLiteralZero {
 public:
  constexpr OnlyLiteralZero(int OnlyLiteralZero::*) noexcept {}

  template <typename T, typename = typename std::enable_if<
                            std::is_same<T, std::nullptr_t>::value ||
                            (std::is_integral<T>::value &&
                             !std::is_same<T, int>::value)>::type>
  OnlyLiteralZero(T) {
    static_assert(sizeof(T) < 0, "Only literal `0` is allowed.");
  }
};

int main() {
  OnlyLiteralZero z(0);
  return 0;
}
