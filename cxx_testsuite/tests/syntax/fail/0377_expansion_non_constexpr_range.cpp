// RUN: -std=c++26
// EXPECT: Expansion statement range is not a constant expression

struct Box {
  int data[3];
  const int *begin() const { return data; }
  const int *end() const { return data + 3; }
};

int main(void) {
  Box box = {{1, 2, 3}};
  int sum = 0;
  template for (auto x : box) {
    sum += x;
  }
  return sum;
}
