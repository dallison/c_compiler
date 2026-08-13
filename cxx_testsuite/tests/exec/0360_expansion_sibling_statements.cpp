// RUN: -std=c++26
// EXPECT_EXIT: 15
int main(void) {
  int a = 0;
  int b = 0;
  template for (auto x : {1, 2, 3}) {
    a += x;
  }
  template for (auto y : {4, 5}) {
    b += y;
  }
  return a + b;
}
