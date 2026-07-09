// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>
#include <utility>

int main(void) {
  std::variant<int, long> left(3);
  std::variant<int, long> right(9L);

  std::swap(left, right);
  if (left.index() != 1 || std::get<long>(left) != 9L ||
      right.index() != 0 || std::get<int>(right) != 3) {
    return 1;
  }

  using std::swap;
  swap(left, right);
  if (left.index() != 0 || std::get<int>(left) != 3 ||
      right.index() != 1 || std::get<long>(right) != 9L) {
    return 2;
  }

  return 0;
}
