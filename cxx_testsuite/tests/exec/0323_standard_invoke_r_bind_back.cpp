// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <functional>
#include <type_traits>
#include <utility>

#if __cpp_lib_invoke_r != 202106L
#error "__cpp_lib_invoke_r has the wrong value"
#endif

#if __cpp_lib_bind_back != 202202L
#error "__cpp_lib_bind_back has the wrong value"
#endif

struct accumulator {
  int base;

  int add(int left, int right) const noexcept {
    return base + left + right;
  }
};

struct qualified_callable {
  int operator()(int left, int right) & {
    return 100 + left + right;
  }

  int operator()(int left, int right) const& {
    return 200 + left + right;
  }

  int operator()(int left, int right) && {
    return 300 + left + right;
  }

  int operator()(int left, int right) const&& {
    return 400 + left + right;
  }
};

struct converted {
  int value;

  operator long() const noexcept {
    return value;
  }
};

static int side_effect;

static converted make_converted(int value) noexcept {
  return converted{value};
}

static int record_value(int value) {
  side_effect = value;
  return value + 1;
}

int main() {
  static_assert(std::is_same_v<
                decltype(std::invoke_r<long>(make_converted, 3)), long>);

  if (std::invoke_r<long>(make_converted, 17) != 17) {
    return 1;
  }
  std::invoke_r<void>(record_value, 23);
  if (side_effect != 23) {
    return 2;
  }

  accumulator value{10};
  if (std::invoke_r<int>(&accumulator::add, value, 3, 4) != 17) {
    return 3;
  }

  auto member = std::bind_back(&accumulator::add, 6);
  if (member(value, 5) != 21) {
    return 4;
  }

  int referenced = 7;
  auto update = std::bind_back(
      [](int amount, int& destination) {
        destination += amount;
        return destination;
      },
      std::ref(referenced));
  if (update(8) != 15 || referenced != 15) {
    return 5;
  }

  auto bound = std::bind_back(qualified_callable{}, 2);
  if (bound(1) != 103) {
    return 6;
  }
  const auto const_bound = std::bind_back(qualified_callable{}, 3);
  if (const_bound(1) != 204) {
    return 7;
  }
  if (std::move(bound)(1) != 303) {
    return 8;
  }
  if (std::move(const_bound)(1) != 404) {
    return 9;
  }

  return 0;
}
