// RUN: -std=c++23
// EXPECT_EXIT: 0

struct pair_value {
  int value;

  pair_value(int left, int right) : value(left + right) {}
};

template <class T, class... Args>
T make_value(Args&&... args) {
  T value(static_cast<Args&&>(args)...);
  return value;
}

int main() {
  if (make_value<int>() != 0) {
    return 1;
  }
  if (make_value<int>(42) != 42) {
    return 2;
  }
  if (make_value<pair_value>(19, 23).value != 42) {
    return 3;
  }
  return 0;
}
