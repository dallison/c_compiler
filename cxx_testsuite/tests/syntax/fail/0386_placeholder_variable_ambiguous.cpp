// RUN: -std=c++26
// EXPECT: name-independent declaration '_' is ambiguous

int main() {
  int _ = 1;
  long _ = 2;
  return _;
}
