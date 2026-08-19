// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <meta>

namespace generated {}

consteval auto declaration() {
  return ^{
    constexpr int injected = \(7);
  };
}

consteval void install() {
  std::meta::namespace_inject(^^generated, declaration());
}

int main() {
  consteval { install(); }
  if (generated::injected != 7) {
    return 1;
  }
  return 0;
}
