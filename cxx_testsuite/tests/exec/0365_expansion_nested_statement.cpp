// RUN: -std=c++26
// EXPECT_EXIT: 12

int main() {
  int total = 0;
  template for (auto x : {1, 2}) {
    template for (auto y : {3, 4}) {
      if (y == 3) {
        continue;
      }
      total += x * y;
      if (x == 2) {
        break;
      }
    }
  }
  return total;
}
