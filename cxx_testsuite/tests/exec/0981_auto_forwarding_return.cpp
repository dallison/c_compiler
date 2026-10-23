// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
auto&& pass(T&& value) {
  return static_cast<T&&>(value);
}

int main() {
  int value = 4;
  int& reference = pass(value);
  if (reference != 4) {
    return 1;
  }
  int&& moved = pass(4);
  if (moved != 4) {
    return 2;
  }
  return 0;
}
