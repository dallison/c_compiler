// RUN: -std=c++17
// EXPECT_EXIT: 0

// A virtual call through a pointer to member loads the vtable slot and calls
// it. On the ARM EABI member-pointer ABI that load is retyped as a function
// pointer after analysis; the retype must not use a type analysis already
// released.

struct Base {
  virtual int f(int x) { return x + 1; }
  virtual int g(int x) { return x + 2; }
};

struct Derived : Base {
  int f(int x) override { return x + 3; }
};

int call(Base* object, int (Base::*method)(int), int arg) {
  return (object->*method)(arg);
}

int main() {
  Derived derived;
  Base base;
  if (call(&derived, &Base::f, 4) != 7) {
    return 1;
  }
  if (call(&base, &Base::f, 4) != 5) {
    return 2;
  }
  if (call(&derived, &Base::g, 4) != 6) {
    return 3;
  }
  return 0;
}
