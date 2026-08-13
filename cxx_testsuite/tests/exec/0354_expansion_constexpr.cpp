// RUN: -std=c++26
// EXPECT_EXIT: 0
constexpr int sum_enumerating(void) {
  int sum = 0;
  template for (auto x : {1, 2, 3}) {
    sum += x;
  }
  return sum;
}

int main(void) {
  return sum_enumerating() == 6 ? 0 : 1;
}
