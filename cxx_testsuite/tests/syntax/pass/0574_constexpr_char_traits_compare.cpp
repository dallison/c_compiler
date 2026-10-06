// RUN: -std=c++20 -fconstexpr-eval=audit

// char_traits<char>::compare is constexpr, and a constexpr function that
// happens to be named memcpy may be called in a constant expression.
#include <cstddef>
#include <string>

static_assert(std::char_traits<char>::compare("abc", "abd", 3) < 0);
static_assert(std::char_traits<char>::compare("abd", "abc", 3) > 0);
static_assert(std::char_traits<char>::compare("abc", "abc", 3) == 0);
static_assert(std::char_traits<char>::compare("\x80", "a", 1) > 0);

namespace mine {
constexpr void* memcpy(void* to, const void* from, std::size_t n) {
  (void)from;
  (void)n;
  return to;
}
}  // namespace mine

constexpr bool named_memcpy() {
  int x = 0;
  return mine::memcpy(&x, &x, sizeof x) == &x;
}
static_assert(named_memcpy());
