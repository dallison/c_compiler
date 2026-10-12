// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class T>
auto identity(T value);

template <class T>
auto identity(T value) {
  return value;
}

auto number();

auto number() { return 9; }

int main() { return identity(number()) == 9 ? 0 : 1; }
