// RUN: -std=c++26
// EXPECT: 'main' cannot be a function template

template <typename T>
T main() {
  return T{};
}
