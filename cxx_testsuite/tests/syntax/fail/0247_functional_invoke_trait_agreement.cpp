// RUN: -std=c++20
// EXPECT: static assertion failed
#include <functional>
#include <type_traits>

struct Box {
  int value;
  int scale(int factor) const { return value * factor; }
};

static_assert(std::is_invocable<int (Box::*)(int) const, Box, int, int>::value,
              "static assertion failed");
static_assert(std::is_invocable<int Box::*, int>::value,
              "static assertion failed");
static_assert(std::is_invocable<int (Box::*)(int) const, int, int>::value,
              "static assertion failed");
static_assert(
    std::is_same<std::invoke_result_t<int (Box::*)(int) const, Box, int>,
                 void>::value,
    "static assertion failed");

int main(void) {
  return 0;
}
