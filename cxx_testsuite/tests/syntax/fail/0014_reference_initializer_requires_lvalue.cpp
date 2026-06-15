// RUN: -std=c++17
// EXPECT: Reference initializer must be an lvalue
int main(void) {
  int &ref = 1;
  return sizeof(ref);
}
