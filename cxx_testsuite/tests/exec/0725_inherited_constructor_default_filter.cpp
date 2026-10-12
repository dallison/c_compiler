// RUN: -std=c++17
// EXPECT_EXIT: 0

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

int main() {
  Derived defaulted;
  Derived inherited(35);
  return defaulted.value + inherited.value == 42 ? 0 : 1;
}
