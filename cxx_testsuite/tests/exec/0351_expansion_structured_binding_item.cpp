// RUN: -std=c++26
// EXPECT_EXIT: 10
struct Pair {
  int a;
  int b;
};

int main(void) {
  Pair rows[2] = {{1, 2}, {3, 4}};
  int sum = 0;
  template for (auto [a, b] : rows) {
    sum += a + b;
  }
  return sum;
}
