// RUN: -std=c++26
// EXPECT_EXIT: 6

struct triple {
  int first;
  int second;
  int third;
};

int main() {
  int sum = 0;
  template for (auto value : triple{1, 2, 3}) {
    sum += value;
  }
  return sum;
}
