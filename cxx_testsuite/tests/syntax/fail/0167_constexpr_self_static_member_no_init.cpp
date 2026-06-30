// RUN: -std=c++20
// EXPECT: constexpr variable requires an initializer

// A `static constexpr` data member must be initialized in the class
// definition; declaring one with no initializer is ill-formed even if a later
// out-of-class definition could supply a value.
struct Cat {
  int _v;
  constexpr explicit Cat(signed char v) : _v(v) {}
  static constexpr Cat less;
};
