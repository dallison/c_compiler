// RUN: -std=c++20
// EXPECT: Rvalue reference initializer must not be an lvalue

int main(void) {
  int value = 1;
  int&& ref = value;
  return ref;
}
