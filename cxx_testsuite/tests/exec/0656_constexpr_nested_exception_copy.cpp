// RUN: -std=c++26
// EXPECT_EXIT: 0

// Throwing a class derived from std::nested_exception, with a user copy
// constructor, from inside a handler: the nested exception is captured, and
// the copy constructor runs only where a copy is actually made.  Checked in
// constant evaluation and at run time.
#include <exception>

struct P : std::nested_exception {
  int value;
  constexpr explicit P(int v) : value(v) {}
  constexpr P(const P& o) : std::nested_exception(o), value(o.value + 100) {}
};

constexpr int thrown_prvalue() {
  try {
    try {
      throw 31;
    } catch (...) {
      throw P(11);
    }
  } catch (const P& outer) {
    try {
      outer.rethrow_nested();
    } catch (int inner) {
      return outer.value + inner;
    }
  }
  return 0;
}

constexpr int thrown_lvalue() {
  try {
    try {
      throw 31;
    } catch (...) {
      P p(11);
      throw p;
    }
  } catch (const P& outer) {
    try {
      outer.rethrow_nested();
    } catch (int inner) {
      return outer.value + inner;
    }
  }
  return 0;
}

constexpr int caught_by_value() {
  try {
    try {
      throw 31;
    } catch (...) {
      throw P(11);
    }
  } catch (P outer) {
    try {
      outer.rethrow_nested();
    } catch (int inner) {
      return outer.value + inner;
    }
  }
  return 0;
}

constexpr int nested_in_handler() {
  try {
    try {
      throw 31;
    } catch (...) {
      std::nested_exception n;
      n.rethrow_nested();
    }
  } catch (int inner) {
    return inner;
  }
  return 0;
}

constexpr int nested_ptr_rethrown() {
  try {
    try {
      throw 31;
    } catch (...) {
      std::nested_exception n;
      std::rethrow_exception(n.nested_ptr());
    }
  } catch (int inner) {
    return inner;
  }
  return 0;
}

constexpr int moved_from_current_exception() {
  std::exception_ptr keep;
  try {
    try {
      throw 31;
    } catch (...) {
      keep = std::current_exception();
    }
    std::rethrow_exception(keep);
  } catch (int inner) {
    return inner;
  }
  return 0;
}

struct Q {
  int value;
  constexpr explicit Q(int v) : value(v) {}
  constexpr Q(const Q& o) : value(o.value + 100) {}
};

constexpr int plain_caught_by_value() {
  try {
    throw Q(11);
  } catch (Q o) {
    return o.value;
  }
  return 0;
}

static_assert(thrown_prvalue() == 42);
static_assert(thrown_lvalue() == 142);
static_assert(caught_by_value() == 142);
static_assert(nested_in_handler() == 31);
static_assert(nested_ptr_rethrown() == 31);
static_assert(moved_from_current_exception() == 31);
static_assert(plain_caught_by_value() == 111);

int main() {
  if (thrown_prvalue() != 42) return 1;
  if (thrown_lvalue() != 142) return 2;
  if (caught_by_value() != 142) return 3;
  if (nested_in_handler() != 31) return 4;
  if (nested_ptr_rethrown() != 31) return 5;
  if (moved_from_current_exception() != 31) return 6;
  if (plain_caught_by_value() != 111) return 7;
  return 0;
}
