// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

int main(void) {
  std::variant<int, long> from_int(4);
  if (from_int.index() != 0 || std::get<0>(from_int) != 4) {
    return 1;
  }

  std::variant<int, long> from_long(9L);
  if (from_long.index() != 1 || std::get<1>(from_long) != 9L) {
    return 2;
  }

  from_int = 12L;
  if (from_int.index() != 1 || std::get<long>(from_int) != 12L) {
    return 3;
  }

  std::variant<int, const char*> text("hello");
  if (text.index() != 1 || std::get<const char*>(text)[1] != 'e') {
    return 4;
  }

  std::variant<int, int> duplicate(std::in_place_index<1>, 22);
  if (duplicate.index() != 1 || std::get<1>(duplicate) != 22) {
    return 5;
  }

  duplicate.emplace<0>(17);
  if (duplicate.index() != 0 || std::get<0>(duplicate) != 17) {
    return 6;
  }

  return 0;
}
