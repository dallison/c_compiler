// RUN: -std=c++26 -fconstexpr-eval=audit

#include <exception>

struct nested_error : std::nested_exception {
  int value;

  constexpr explicit nested_error(int v) : value(v) {}
};

constexpr int capture_and_rethrow() {
  try {
    throw 17;
  } catch (...) {
    std::exception_ptr saved = std::current_exception();
    try {
      std::rethrow_exception(saved);
    } catch (int value) {
      return value;
    }
  }
  return 0;
}

constexpr int make_and_rethrow() {
  std::exception_ptr saved = std::make_exception_ptr(29);
  try {
    std::rethrow_exception(saved);
  } catch (int value) {
    return value;
  }
  return 0;
}

constexpr int nested_rethrow() {
  try {
    try {
      throw 31;
    } catch (...) {
      std::throw_with_nested(nested_error(11));
    }
  } catch (const nested_error& outer) {
    try {
      outer.rethrow_nested();
    } catch (int inner) {
      return outer.value + inner;
    }
  }
  return 0;
}

struct unwind_probe {
  int* observed;

  constexpr ~unwind_probe() {
    *observed = std::uncaught_exceptions();
  }
};

constexpr int check_uncaught_count() {
  int observed = 0;
  try {
    unwind_probe probe{&observed};
    throw 1;
  } catch (...) {
  }
  return observed;
}

struct replacement_probe {
  int* copies_alive;
  bool is_copy;

  constexpr explicit replacement_probe(int* count)
      : copies_alive(count), is_copy(false) {}

  constexpr replacement_probe(const replacement_probe& other)
      : copies_alive(other.copies_alive), is_copy(true) {
    ++*copies_alive;
  }

  constexpr ~replacement_probe() {
    if (is_copy) {
      --*copies_alive;
    }
  }
};

constexpr bool replaced_exception_is_destroyed() {
  int copies_alive = 0;
  try {
    try {
      throw replacement_probe(&copies_alive);
    } catch (...) {
      throw 2;
    }
  } catch (int) {
    return copies_alive == 0;
  }
  return false;
}

static_assert(capture_and_rethrow() == 17);
static_assert(make_and_rethrow() == 29);
static_assert(nested_rethrow() == 42);
static_assert(check_uncaught_count() == 1);
static_assert(replaced_exception_is_destroyed());
