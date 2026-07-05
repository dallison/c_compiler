// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

struct Visitor {
  int operator()(int value) const {
    return value + 1;
  }

  int operator()(long value) const {
    return (int)value + 10;
  }
};

int main(void) {
  std::variant<int, long> value(4);
  if (std::visit(Visitor{}, value) != 5) {
    return 1;
  }

  value.emplace<1>(7L);
  if (std::visit(Visitor{}, value) != 17) {
    return 2;
  }

  const std::variant<int, long> const_value(9);
  if (std::visit(Visitor{}, const_value) != 10) {
    return 3;
  }

  return 0;
}
