// RUN: -std=c++20

// Exception specifications are accepted by the front end.  A throw inside a
// noexcept function is well-formed (it triggers std::terminate at runtime if it
// escapes), so none of these forms is a compile-time error.

// A noexcept function with no throw is fine.
void never_throws(void) noexcept {
}

// noexcept(true) / noexcept(1) are non-throwing but contain no throw.
void noexcept_true(void) noexcept(true) {
}

void noexcept_constant(void) noexcept(1) {
}

// noexcept(false) functions are permitted to throw.
int may_throw(bool cond) noexcept(false) {
  if (cond) {
    throw 1;
  }
  return 0;
}

// A throw inside a try block does not escape, so it is allowed even in a
// noexcept function.
void caught_locally(void) noexcept {
  try {
    throw 2;
  } catch (...) {
  }
}

// Functions with no exception specification may throw freely.
int throws_freely(void) {
  throw 3;
}

// The deprecated dynamic specification throw(int) permits throwing.
void dynamic_spec(void) throw(int) {
  throw 4;
}

// Lambdas without noexcept may throw.
int throwing_lambda(void) {
  return []() { return throw 5, 0; }();
}
