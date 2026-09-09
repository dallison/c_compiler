// RUN: -std=c++11
// EXPECT: local variable in a constexpr function requires C++14

constexpr int with_local() {
  int x = 1;
  return x;
}
