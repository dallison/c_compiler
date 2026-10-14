// RUN: -std=c++17 -O2
// EXPECT_EXIT: 0

// Inheriting a defaulted constructor gives the derived class a copied function
// type whose body still refers to the base constructor's parameters.

struct Root {
  int value;

  Root() : value(7) {}
  explicit Root(int value) : value(value) {}
};

struct Base : Root {
  Base() = default;
  using Root::Root;
};

struct Derived : Base {
  using Base::Base;
};

struct Leaf : Derived {
  using Derived::Derived;
};

int main() {
  Derived defaulted;
  Derived inherited(35);
  Leaf leaf;
  return defaulted.value + inherited.value == 42 && leaf.value == 7 ? 0 : 1;
}
