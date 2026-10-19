// RUN: -std=c++20
// EXPECT: Illegal conversion; cannot convert from

// An enumerator with value zero is not an integer literal.
enum E { kZero };

int main() {
  int* p = kZero;
  return p == nullptr ? 0 : 1;
}
