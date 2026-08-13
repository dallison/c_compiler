// RUN: -std=c++26
// EXPECT: label declared in an expansion statement body is not permitted

int labeled_expansion(void) {
  int sum = 0;
  template for (auto x : {1, 2, 3}) {
  label:
    sum += x;
  }
  return sum;
}
