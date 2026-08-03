// RUN: -std=c++20 -fconstexpr-eval=pcode

struct virtual_base {
  constexpr virtual int value() const { return 1; }
};

struct virtual_derived : virtual_base {
  constexpr int value() const override { return 42; }
};

constexpr int dispatch() {
  virtual_derived object;
  virtual_base* base = &object;
  return base->value();
}

constexpr int dispatch_again() {
  virtual_derived object;
  virtual_base* base = &object;
  return base->value() + 1;
}

static_assert(dispatch() == 42);
static_assert(dispatch_again() == 43);
