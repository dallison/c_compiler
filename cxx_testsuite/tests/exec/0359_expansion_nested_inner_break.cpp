// RUN: -std=c++26
// EXPECT_EXIT: 6
int main(void) {
  int count = 0;
  template for (auto x : {1, 2, 3}) {
    for (int i = 0; i < 3; ++i) {
      if (x == 2) {
        break;
      }
      count++;
    }
  }
  return count;
}
