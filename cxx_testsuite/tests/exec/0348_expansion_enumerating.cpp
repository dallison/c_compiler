// RUN: -std=c++26
// EXPECT_EXIT: 6
int main() {
  int sum = 0;
  template for (auto x : {1, 2, 3}) {
    sum += x;
  }
  return sum;
}
