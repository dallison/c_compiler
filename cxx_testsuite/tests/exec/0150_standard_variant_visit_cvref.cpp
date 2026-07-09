// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>
#include <utility>

struct RvalueOnly {
  int operator()(int&) const {
    return 0;
  }

  int operator()(int&& value) const {
    return value == 40 ? 6 : 0;
  }

  int operator()(long&) const {
    return 0;
  }

  int operator()(long&& value) const {
    return value == 40L ? 7 : 0;
  }
};

struct RvalueMutates {
  int operator()(int&) const {
    return 0;
  }

  int operator()(int&& value) const {
    value = 41;
    return 12;
  }

  int operator()(long&) const {
    return 0;
  }

  int operator()(long&& value) const {
    value = 42L;
    return 13;
  }
};

struct ConstRvalueOnly {
  int operator()(const int&) const {
    return 0;
  }

  int operator()(const int&& value) const {
    return value == 50 ? 8 : 0;
  }

  int operator()(const long&) const {
    return 0;
  }

  int operator()(const long&& value) const {
    return value == 50L ? 9 : 0;
  }
};

struct ThirdRvalueOnly {
  int operator()(int, long, int&) const {
    return 0;
  }

  int operator()(int first, long second, int&& third) const {
    return first == 5 && second == 6L && third == 70 ? 10 : 0;
  }

  int operator()(int, long, long&) const {
    return 0;
  }

  int operator()(int first, long second, long&& third) const {
    return first == 5 && second == 6L && third == 70L ? 11 : 0;
  }
};

int main(void) {
  std::variant<int, long> value(10);
  if (std::visit([](auto item) { return item == 10 ? 1 : 0; }, value) != 1) {
    return 1;
  }

  const std::variant<int, long> const_value(20);
  if (std::visit([](auto item) { return item == 20 ? 2 : 0; }, const_value) !=
      2) {
    return 2;
  }

  std::variant<int, long> rvalue(30);
  if (std::visit([](auto item) { return item == 30 ? 3 : 0; },
                 std::move(rvalue)) != 3) {
    return 3;
  }

  std::variant<int, long> first(5);
  const std::variant<int, long> second(std::in_place_index<1>, 6L);
  std::variant<int, long> third(7);
  if (std::visit([](auto a, auto b, auto c) {
        return a == 5 && b == 6L && c == 7 ? 5 : 0;
      }, first, second, third) != 5) {
    return 4;
  }

  std::variant<int, long> moved(40);
  if (std::visit(RvalueOnly{}, std::move(moved)) != 6) {
    return 5;
  }

  std::variant<int, long> mutated(40);
  if (std::visit(RvalueMutates{}, std::move(mutated)) != 12 ||
      std::get<0>(mutated) != 41) {
    return 6;
  }

  const std::variant<int, long> const_moved(50);
  if (std::visit(ConstRvalueOnly{}, std::move(const_moved)) != 8) {
    return 7;
  }

  std::variant<int, long> multi_moved(70);
  if (std::visit(ThirdRvalueOnly{}, first, second, std::move(multi_moved)) !=
      10) {
    return 8;
  }

  return 0;
}
