// RUN: -std=c++23
// EXPECT: Built-in subscripting requires exactly one index

int values[] = {1, 2, 3};

int read() {
  return values[0, 1];
}
