// RUN: -std=c++26
// EXPECT_EXIT: 3
template <typename... Ts>
int sum_until(Ts... vals) {
  int sum = 0;
  template for (auto x : {vals...}) {
    if (x == 3) {
      break;
    }
    sum += x;
  }
  return sum;
}

int main(void) {
  return sum_until(1, 2, 3, 4);
}
