// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>

#ifndef __cpp_lib_move_only_function
#error "__cpp_lib_move_only_function is required"
#endif

static_assert(__cpp_lib_move_only_function == 202110L);

struct immovable_target {
  int offset;

  explicit immovable_target(int value) : offset(value) {}
  immovable_target(const immovable_target&) = delete;
  immovable_target(immovable_target&&) = delete;

  int operator()(int value) {
    return offset + value;
  }
};

struct list_target {
  int total;

  list_target(std::initializer_list<int> values, int extra) : total(extra) {
    for (int value : values) {
      total += value;
    }
  }

  int operator()() {
    return total;
  }
};

struct move_target {
  int value;

  explicit move_target(int initial) : value(initial) {}
  move_target(const move_target&) = delete;
  move_target(move_target&& other) noexcept : value(other.value) {
    other.value = -1;
  }

  int operator()(int argument) {
    return value + argument;
  }
};

struct lvalue_target {
  int operator()(int value) & {
    return value + 1;
  }
};

struct rvalue_target {
  int operator()(int value) && {
    return value + 2;
  }
};

struct const_lvalue_target {
  int operator()(int value) const& {
    return value + 3;
  }
};

struct const_rvalue_target {
  int operator()(int value) const&& noexcept {
    return value + 4;
  }
};

int plus_five(int value) {
  return value + 5;
}

static_assert(std::is_invocable_r<int, move_target&, int>::value);
static_assert(std::is_invocable_r<int, rvalue_target&&, int>::value);
static_assert(std::is_nothrow_invocable_r<
              int, const const_rvalue_target&&, int>::value);
static_assert(std::is_invocable_r<int, lvalue_target&, int>::value);
static_assert(std::is_invocable_r<
              int, const const_lvalue_target&, int>::value);
static_assert(std::is_constructible<
              list_target, std::initializer_list<int>&, int&&>::value);

int main() {
  std::move_only_function<int(int)> empty;
  if (empty || empty != nullptr) {
    return 1;
  }

  std::move_only_function<int(int)> pointer = &plus_five;
  if (!pointer || pointer(2) != 7) {
    return 2;
  }

  int (*null_pointer)(int) = nullptr;
  std::move_only_function<int(int)> null_function = null_pointer;
  if (null_function != nullptr) {
    return 3;
  }

  std::move_only_function<int(int)> moved_from =
      move_target(10);
  std::move_only_function<int(int)> moved_to =
      static_cast<std::move_only_function<int(int)>&&>(moved_from);
  if (moved_from || moved_to(3) != 13) {
    return 4;
  }

  moved_to = nullptr;
  if (moved_to) {
    return 5;
  }

  std::move_only_function<int(int)> immovable(
      std::in_place_type<immovable_target>, 20);
  if (immovable(2) != 22) {
    return 6;
  }

  std::move_only_function<int()> from_list(
      std::in_place_type<list_target>, {1, 2, 3}, 4);
  if (from_list() != 10) {
    return 7;
  }

  std::move_only_function<int(int) &> lvalue =
      lvalue_target();
  if (lvalue(10) != 11) {
    return 8;
  }

  std::move_only_function<int(int) &&> rvalue =
      rvalue_target();
  if (static_cast<decltype(rvalue)&&>(rvalue)(10) != 12) {
    return 9;
  }

  const std::move_only_function<int(int) const &> const_lvalue =
      const_lvalue_target();
  if (const_lvalue(10) != 13) {
    return 10;
  }

  const std::move_only_function<int(int) const && noexcept> const_rvalue =
      const_rvalue_target();
  static_assert(noexcept(
      static_cast<decltype(const_rvalue)&&>(const_rvalue)(10)));
  if (static_cast<decltype(const_rvalue)&&>(const_rvalue)(10) != 14) {
    return 11;
  }

  std::move_only_function<int(int)> assigned;
  assigned = move_target(30);
  if (assigned(2) != 32) {
    return 12;
  }

  std::move_only_function<int(int)> swapped = &plus_five;
  assigned.swap(swapped);
  if (assigned(1) != 6 || swapped(1) != 31) {
    return 13;
  }

  return 0;
}
