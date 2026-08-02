// RUN: -std=c++23
// EXPECT: a functional-style cast to auto requires exactly one argument

int main() {
  int first = 1;
  int second = 2;
  return auto{first, second};
}
