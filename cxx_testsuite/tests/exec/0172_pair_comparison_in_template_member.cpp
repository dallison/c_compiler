// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <utility>

template <class T>
struct Box {
  using value_type = std::pair<const T, T>;

  bool same(const value_type& left, const value_type& right) const {
    return left == right;
  }

  auto compare(const value_type& left, const value_type& right) const {
    return left <=> right;
  }
};

int main() {
  Box<int> box;
  std::pair<const int, int> left(1, 2);
  std::pair<const int, int> same(1, 2);
  std::pair<const int, int> larger(1, 3);
  if (!box.same(left, same) || box.same(left, larger)) {
    return 1;
  }
  return (box.compare(left, larger) < 0) ? 0 : 2;
}
