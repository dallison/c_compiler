// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <cstddef>
#include <type_traits>
#include <vector>

template <typename T>
struct Matcher {};

static int terminator_hit;

template <typename... Args>
struct VariadicMatcher {
  template <typename T>
  void go() const {
    std::vector<Matcher<T>> values;
    Create<T>(&values, std::integral_constant<size_t, 0>());
  }

  template <typename T, size_t I>
  void Create(std::vector<Matcher<T>>* values,
              std::integral_constant<size_t, I>) const {
    (void)values;
    Create<T>(values, std::integral_constant<size_t, I + 1>());
  }

  template <typename T>
  void Create(std::vector<Matcher<T>>*,
              std::integral_constant<size_t, sizeof...(Args)>) const {
    terminator_hit = 1;
  }
};

int main() {
  VariadicMatcher<int, int> matcher;
  matcher.go<char>();
  if (terminator_hit != 1) return 1;
  return 0;
}
