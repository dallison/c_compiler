// RUN: -std=c++26
// EXPECT: Symbol _ redeclared with different linkage

int main() {
  int _;
  static int _;
  return 0;
}
