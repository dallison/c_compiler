// RUN: -std=c++26
// EXPECT_EXIT: 36
int main(void) {
  int sum = 0;
  template for (auto x : {1, 2, 3}) {
    int acc = 0;
    for (int i = 0; i < x; ++i) {
      acc++;
    }
    sum += acc;
    {
      int x = 10;
      sum += x;
    }
  }
  return sum;
}
