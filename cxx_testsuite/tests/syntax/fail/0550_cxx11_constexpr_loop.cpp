// RUN: -std=c++11
// EXPECT: statement is not allowed in a C++11 constexpr function

constexpr int with_loop(int n) {
  while (n > 0) {
    --n;
  }
  return n;
}
