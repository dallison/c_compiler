// RUN: -std=c++20
// EXPECT: Use of implicitly deleted function Derived

struct Base {
  Base();
  Base(const Base& other) = delete;
};

Base::Base() {
}

struct Derived : public Base {
  int value;
  Derived();
};

Derived::Derived() {
  value = 7;
}

void fail_derived_implicit_copy() {
  Derived first;
  Derived second(first);
}
