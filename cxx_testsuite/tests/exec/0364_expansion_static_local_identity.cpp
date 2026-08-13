// RUN: -std=c++26
// EXPECT_EXIT: 6
int main(void) {
  int sum = 0;
  template for (auto x : {1, 2, 3}) {
    static int counter = 0;
    counter += x;
    sum = counter;
  }
  return sum;
}
