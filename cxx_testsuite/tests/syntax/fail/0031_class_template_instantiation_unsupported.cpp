// RUN: -std=c++20
// EXPECT: Class template instantiation is not supported yet

template <int N>
struct Sized {
  int value;
};

Sized<int> object;
