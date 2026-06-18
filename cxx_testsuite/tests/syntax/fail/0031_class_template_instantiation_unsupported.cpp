// RUN: -std=c++20
// EXPECT: Template non-type argument must be an integer constant expression

template <int N>
struct Sized {
  int value;
};

Sized<int> object;
