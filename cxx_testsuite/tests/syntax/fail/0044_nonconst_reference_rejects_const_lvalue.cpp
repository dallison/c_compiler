// RUN: -std=c++20
// EXPECT: Reference initializer discards qualifiers

int source(void);

int main(void) {
  const int value = source();
  int& ref = value;
  return ref;
}
