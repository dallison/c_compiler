// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <functional>
#include <type_traits>

struct Adder {
  int base;

  explicit Adder(int initial) : base(initial) {}

  int operator()(int value) const {
    return base + value;
  }
};

int times_two(int value) {
  return value * 2;
}

int main() {
  Adder adder(3);
  if (std::invoke(adder, 4) != 7) {
    return 1;
  }

  std::reference_wrapper<Adder> wrapped = std::ref(adder);
  if (std::invoke(wrapped, 5) != 8) {
    return 2;
  }

  if (std::invoke(times_two, 6) != 12) {
    return 4;
  }

  if (!std::is_invocable<Adder, int>::value) {
    return 5;
  }
  if (std::is_invocable<Adder>::value) {
    return 6;
  }
  if (!std::is_same<std::invoke_result_t<Adder, int>, int>::value) {
    return 7;
  }

  return 0;
}
