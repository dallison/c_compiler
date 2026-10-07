// RUN: -std=c++20 -fconstexpr-eval=audit

// std::construct_at value-initializes a scalar union member when given no
// arguments, and a member union becomes active by constructing it before an
// object is constructed inside it.

#include <memory>

struct P {
  int a;
  constexpr P(int x) : a(x) {}
};

union Inner {
  char d;
  P p;
  constexpr Inner() : d(0) {}
  constexpr ~Inner() {}
};

union Outer {
  char d;
  int i;
  Inner rest;
  constexpr Outer() : d(0) {}
  constexpr ~Outer() {}
};

constexpr int ValueInit() {
  Outer o;
  std::construct_at(&o.i);
  return o.i;
}

constexpr int ValueInitThenSwitch() {
  Outer o;
  std::construct_at(&o.i);
  std::construct_at(&o.rest);
  std::construct_at(&o.rest.p, 8);
  return o.rest.p.a;
}

constexpr int Direct() {
  Outer o;
  std::construct_at(&o.i, 3);
  return o.i;
}

constexpr int Nested() {
  Outer o;
  std::construct_at(&o.rest);
  std::construct_at(&o.rest.p, 5);
  return o.rest.p.a;
}

static_assert(ValueInit() == 0);
static_assert(ValueInitThenSwitch() == 8);
static_assert(Direct() == 3);
static_assert(Nested() == 5);
