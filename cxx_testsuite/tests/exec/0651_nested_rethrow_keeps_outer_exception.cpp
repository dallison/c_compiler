// RUN: -std=c++20
// EXPECT_EXIT: 0

// Rethrowing out of a handler nested inside another handler ends only the
// inner handler; the outer caught exception stays alive and current.
#include <exception>

static int alive = 0;
struct A {
  int v;
  A(int v) : v(v) { ++alive; }
  A(const A& o) : v(o.v) { ++alive; }
  ~A() { --alive; }
};
struct B {
  int v;
};

int main() {
  try {
    throw A(5);
  } catch (A& outer) {
    try {
      try {
        throw B{3};
      } catch (B&) {
        throw;
      }
    } catch (B& b) {
      if (b.v != 3) return 1;
    }
    if (alive != 1) return 2;
    if (std::uncaught_exceptions() != 0) return 3;
    try {
      throw;
    } catch (A& again) {
      if (&again != &outer || again.v != 5) return 4;
    }
    if (alive != 1) return 5;
    try {
      std::rethrow_exception(std::current_exception());
    } catch (A& again) {
      if (&again != &outer) return 6;
    }
    if (alive != 1) return 7;
  }
  if (alive != 0) return 8;
  return 0;
}
