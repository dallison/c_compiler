// RUN: -std=c++20
// EXPECT: Rvalue reference argument must not be an lvalue

int read_ref(int&& value) {
  return value;
}

int main(void) {
  int value = 1;
  return read_ref(value);
}
