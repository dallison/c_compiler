// RUN: -std=c++20
// EXPECT: auto(x) and auto{x} require C++23

int main() {
  int value = 1;
  return auto(value);
}
