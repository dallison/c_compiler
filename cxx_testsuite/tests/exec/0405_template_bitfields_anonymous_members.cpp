// RUN: -std=c++20
// EXPECT_EXIT: 0

template <typename T>
struct Record {
  T prefix;
  unsigned low : 4;
  unsigned high : 4;
  struct {
    T anonymous_value;
    unsigned tail : 3;
  };
};

int main() {
  Record<int> value{};
  value.prefix = 3;
  value.low = 7;
  value.high = 11;
  value.anonymous_value = 13;
  value.tail = 5;

  if (value.prefix != 3 || value.low != 7 || value.high != 11) {
    return 1;
  }
  if (value.anonymous_value != 13 || value.tail != 5) {
    return 2;
  }
  return 0;
}
