// RUN: -std=c++26
// EXPECT_EXIT: 6
template <typename... Ts>
int sum_pack(Ts... vals) {
  int sum = 0;
  template for (auto x : {vals...}) {
    sum += x;
  }
  return sum;
}

int main(void) {
  return sum_pack(1, 2, 3);
}
