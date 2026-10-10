// RUN: -std=c++26

// A class prvalue passed by value initializes the parameter object, which the
// caller destroys at the end of the full-expression, also when the callee
// exits by an exception.
#include <exception>

struct Counted {
  int* count;
  constexpr Counted(int* c) : count(c) {}
  constexpr Counted(const Counted& o) : count(o.count) { ++*count; }
  constexpr ~Counted() { --*count; }
};

struct Holder {
  Counted c;
  constexpr Counted get() const { return c; }
};

constexpr int observe(Counted c) { return *c.count; }
constexpr int fail(Counted) { throw 7; }

constexpr int prvalue_argument() {
  int n = 1;
  Holder h{Counted(&n)};
  int inside = observe(h.get());
  return inside * 10 + n;
}
static_assert(prvalue_argument() == 21);

constexpr int prvalue_argument_throws() {
  int n = 1;
  Holder h{Counted(&n)};
  try {
    fail(h.get());
  } catch (int) {
  }
  return n;
}
static_assert(prvalue_argument_throws() == 1);

constexpr int lvalue_argument_throws() {
  int n = 1;
  Holder h{Counted(&n)};
  try {
    fail(h.c);
  } catch (int) {
  }
  return n;
}
static_assert(lvalue_argument_throws() == 1);

// Each recursive call materializes its own argument temporary from the same
// expression; every one of them is destroyed exactly once.
constexpr Counted make(int* n) { return Counted(n); }
constexpr int recurse(Counted c, int depth) {
  return depth == 0 ? *c.count : recurse(make(c.count), depth - 1);
}
constexpr int recursive_arguments() {
  int n = 0;
  int innermost = recurse(make(&n), 3);
  return innermost * 10 - n;
}
static_assert(recursive_arguments() == 4);

constexpr int rethrow_prvalue_exception_ptr() {
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
static_assert(rethrow_prvalue_exception_ptr() == 31);

int main() { return 0; }
