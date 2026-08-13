// RUN: -std=c++26
// EXPECT_EXIT: 12

int kind(int) { return 1; }
int kind(long) { return 2; }

int main() {
  int result = 0;
  template for (auto value : {1, 2L}) {
    result = result * 10 + kind(value);
  }
  return result;
}
