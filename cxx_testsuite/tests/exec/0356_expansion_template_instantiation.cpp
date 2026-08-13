// RUN: -std=c++26
// EXPECT_EXIT: 6
template <int... Is>
int sum_ints(void) {
  int sum = 0;
  template for (auto x : {Is...}) {
    sum += x;
  }
  return sum;
}

int main(void) {
  return sum_ints<1, 2, 3>();
}
