// RUN: -std=c++23
// EXPECT: a functional-style cast to auto requires exactly one argument

int main() {
  return auto();
}
