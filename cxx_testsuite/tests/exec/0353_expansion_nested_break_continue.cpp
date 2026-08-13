// RUN: -std=c++26
// EXPECT_EXIT: 3
int main(void) {
  int count = 0;
  template for (auto x : {1, 2, 3}) {
    for (int i = 0; i < 2; ++i) {
      if (x == 2 && i == 1) {
        continue;
      }
      if (x == 3) {
        break;
      }
      count++;
    }
  }
  return count;
}
