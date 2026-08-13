// RUN: -std=c++26
// EXPECT_EXIT: 9

struct range {
  int values[3];

  constexpr const int* begin() const { return values; }
  constexpr const int* end() const { return values + 3; }
};

int main() {
  constexpr range values{{2, 3, 4}};
  static_assert(*values.begin() == 2);
  int sum = 0;
  template for (constexpr int value : values) {
    sum += value;
  }
  return sum;
}
