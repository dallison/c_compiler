// RUN: -std=c++20
// EXPECT: non-static data member cannot be constexpr

// Only static data members can be constexpr.
struct S {
  static constexpr int ok = 1;
  constexpr int size;
};
