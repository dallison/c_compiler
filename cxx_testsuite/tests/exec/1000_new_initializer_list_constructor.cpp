// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <initializer_list>

static int n;

template <class T>
struct Set {
  template <class U>
  Set(std::initializer_list<U> init) {
    n = (int)init.size();
  }
  Set() { n = 7; }
};

struct Wrap : Set<int> {
  Wrap() { n = 7; }
  using Set<int>::Set;
};

struct Pair {
  Pair(int a, int b) : a(a), b(b) {}
  int a;
  int b;
};

int main() {
  Wrap* listed = new Wrap{"a", "b"};
  if (n != 2 || listed == 0) {
    return 1;
  }
  Wrap* empty = new Wrap{};
  if (n != 7 || empty == 0) {
    return 2;
  }
  Pair* pair = new Pair{1, 2};
  if (pair->a + pair->b != 3) {
    return 3;
  }
  return 0;
}
