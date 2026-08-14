// RUN: -std=c++26 -fconstexpr-eval=pcode

struct base_error {
  int code;
};

struct derived_error : base_error {
  int detail;
};

constexpr void throw_selected(int selector) {
  if (selector == 1) {
    throw 42;
  }
  if (selector == 2) {
    throw 3.5;
  }
  derived_error value{{17}, 9};
  throw value;
}

constexpr int cross_frame_dispatch(int selector) {
  try {
    throw_selected(selector);
  } catch (int value) {
    return value;
  } catch (double value) {
    return value == 3.5 ? 35 : -1;
  } catch (const base_error& value) {
    return value.code;
  }
}

constexpr int nested_rethrow() {
  try {
    try {
      throw 73;
    } catch (int) {
      throw;
    }
  } catch (int value) {
    return value;
  }
  return 0;
}

constexpr int catch_and_return_from_callee() {
  try {
    throw 5;
  } catch (int value) {
    return value;
  }
}

constexpr int continue_after_callee_catch() {
  int first = catch_and_return_from_callee();
  try {
    throw 6;
  } catch (int second) {
    return first + second;
  }
}

constexpr int nested_exception_stack() {
  try {
    throw 41;
  } catch (int outer) {
    try {
      throw 1.5;
    } catch (double) {
    }
    try {
      throw;
    } catch (int rethrown) {
      return outer + rethrown;
    }
  }
}

static_assert(cross_frame_dispatch(1) == 42);
static_assert(cross_frame_dispatch(2) == 35);
static_assert(cross_frame_dispatch(3) == 17);
static_assert(nested_rethrow() == 73);
static_assert(continue_after_callee_catch() == 11);
static_assert(nested_exception_stack() == 82);
