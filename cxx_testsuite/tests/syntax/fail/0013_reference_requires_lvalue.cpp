// RUN: -std=c++17
// EXPECT: Reference argument must be an lvalue
int read_ref(int &value) {
  return value;
}

int main(void) {
  return sizeof(read_ref(1));
}
