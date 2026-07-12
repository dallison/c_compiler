// RUN: -std=c++20
// EXPECT_EXIT: nonzero

int recursive_initializer();

int recursive_initializer() {
  static int value = recursive_initializer();
  return value;
}

int main() {
  return recursive_initializer();
}
