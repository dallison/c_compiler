// RUN: -std=c++20
// A virtual function marked 'final' cannot be overridden in a derived class.
// EXPECT: overrides final function

struct Base {
  virtual int f() final { return 1; }
};

struct Derived : public Base {
  int f() override { return 2; }
};
