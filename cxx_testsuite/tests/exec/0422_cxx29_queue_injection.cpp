// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <meta>

namespace generated {
consteval {
  std::meta::queue_injection(
      ^{
        using injected_type = int;
        constexpr injected_type \id("answer", 29) = \(42);
        constexpr int injected_function() { return \(42); }
      });
}
}

int main() {
  return generated::answer29 != 42 || generated::injected_function() != 42;
}
