// RUN: -std=c++26
// EXPECT_EXIT: 6
int main() {
  int arr[3] = {1, 2, 3};
  int sum = 0;
  template for (auto x : arr) {
    sum += x;
  }
  return sum;
}
