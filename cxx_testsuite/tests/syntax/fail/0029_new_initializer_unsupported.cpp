// RUN: -std=c++17
// EXPECT: new initializer for non-class type requires one expression
int main(void) {
  int* value = new int(1, 2);
  return sizeof(value);
}
