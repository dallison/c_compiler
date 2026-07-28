// RUN: -std=c++20
// EXPECT: consteval function call is not a constant expression

#include <format>

void test() {
  (void)std::format("{", 1);
}
