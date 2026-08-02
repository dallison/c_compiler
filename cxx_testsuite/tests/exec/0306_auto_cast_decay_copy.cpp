// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <type_traits>
#include <utility>

struct tracked {
  int value;

  constexpr explicit tracked(int input) : value(input) {}
  constexpr tracked(const tracked& other) : value(other.value + 10) {}
  constexpr tracked(tracked&& other) : value(other.value + 20) {
    other.value = 0;
  }
};

int plus_one(int value) {
  return value + 1;
}

using function_pointer = int (*)(int);

template <class T>
auto dependent_decay_copy(T&& value) {
  return auto(static_cast<T&&>(value));
}

template <class T>
auto dependent_braced_copy(T&& value) {
  return auto{static_cast<T&&>(value)};
}

constexpr int constant = 7;
using braced_constant_type = decltype(auto{constant});
static_assert(auto(constant) == 7);
static_assert(auto{constant} == 7);
static_assert(std::is_same<decltype(auto(constant)), int>::value);
static_assert(std::is_same<braced_constant_type, int>::value);

int main() {
  int value = 3;
  const int const_value = 4;
  int values[2] = {5, 6};

  static_assert(std::is_same<decltype(auto(value)), int>::value);
  static_assert(std::is_same<decltype(auto(const_value)), int>::value);
  static_assert(std::is_same<decltype(auto(values)), int*>::value);
  static_assert(
      std::is_same<decltype(auto(plus_one)), function_pointer>::value);

  if (auto(value) != 3 || auto{const_value} != 4) {
    return 1;
  }
  if (auto(values) != values || auto(values)[1] != 6) {
    return 2;
  }
  if (auto{values} != values) {
    return 3;
  }
  if (auto(plus_one)(8) != 9) {
    return 4;
  }

  tracked source(1);
  if (auto(source).value != 11 || source.value != 1) {
    return 5;
  }
  if (auto(std::move(source)).value != 21 || source.value != 0) {
    return 6;
  }
  if (auto(tracked(8)).value != 8) {
    return 7;
  }
  if (auto{tracked(9)}.value != 9) {
    return 8;
  }

  if (dependent_decay_copy(value) != 3 ||
      dependent_braced_copy(value) != 3) {
    return 9;
  }
  static_assert(
      std::is_same<decltype(dependent_decay_copy(values)), int*>::value);
  if (dependent_decay_copy(values) != values) {
    return 10;
  }

  if ((auto(value)) != 3 || auto((value, const_value)) != 4) {
    return 11;
  }
  if (auto(value)) {
    return 0;
  }
  return 12;
}
