// RUN: -std=c++23

struct Base {};

struct Derived : virtual Base {
  constexpr Derived() {}
  constexpr ~Derived() {}
};
