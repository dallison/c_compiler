// RUN: -std=c++26
// EXPECT_EXIT: 0
int main(void) {
  int count = 0;
  template for (auto x : {}) {
    count += x;
  }
  return count;
}
