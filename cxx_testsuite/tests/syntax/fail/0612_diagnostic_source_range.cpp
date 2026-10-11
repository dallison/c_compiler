// RUN: -std=c++14
// EXPECT: Cannot call a non-function
// EXPECT: return value();
// EXPECT: ^^^^^

int value;

int use() {
  return value();
}
