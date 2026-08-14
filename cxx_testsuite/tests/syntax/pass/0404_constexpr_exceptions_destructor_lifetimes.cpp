// RUN: -std=c++26

struct guard {
  int* state;

  constexpr ~guard() { *state += 1; }
};

struct error {
  int* state;

  constexpr ~error() { *state += 1; }
};

constexpr int locals_are_destroyed_before_the_handler() {
  int state = 0;
  try {
    guard first{&state};
    guard second{&state};
    throw 1;
  } catch (int) {
    return state;
  }
}

constexpr int exception_object_is_destroyed_after_the_handler() {
  int state = 0;
  try {
    error value{&state};
    throw value;
  } catch (const error&) {
    if (state != 1) {
      return -1;
    }
  }
  return state;
}

constexpr int catch_by_value_has_its_own_lifetime() {
  int state = 0;
  try {
    error value{&state};
    throw value;
  } catch (error) {
    if (state != 1) {
      return -1;
    }
  }
  return state;
}

static_assert(locals_are_destroyed_before_the_handler() == 2);
static_assert(exception_object_is_destroyed_after_the_handler() == 2);
static_assert(catch_by_value_has_its_own_lifetime() == 3);
