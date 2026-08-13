// RUN: -std=c++26
// EXPECT_EXIT: 4
int main(void) {
  int outer = 0;
  template for (int i = 0; auto x : {1, 2, 3}) {
    outer += i;
    i += x;
  }
  return outer;
}
