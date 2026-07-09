// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <variant>

int main(void) {
  std::variant<int, long> value(4);
  long widened = std::visit<long>([](auto item) { return item + 3; }, value);
  if (widened != 7L) {
    return 1;
  }

  value.emplace<1>(8L);
  int seen = 0;
  std::visit<void>([&](auto item) { seen = (int)item; }, value);
  if (seen != 8) {
    return 2;
  }

  const std::variant<int, long> const_value(11);
  int narrowed = std::visit<int>([](auto item) { return item; }, const_value);
  if (narrowed != 11) {
    return 3;
  }

  std::variant<int, long> left(5);
  std::variant<int, long> right(6L);
  long multi = std::visit<long>(
      [](auto a, auto b) { return (long)a + (long)b; }, left, right);
  if (multi != 11L) {
    return 4;
  }

  seen = 0;
  std::visit<void>(
      [&](auto a, auto b) { seen = (int)((long)a * (long)b); }, left, right);
  if (seen != 30) {
    return 5;
  }

  std::variant<int, long> third(7);
  long triple = std::visit<long>(
      [](auto a, auto b, auto c) {
        return (long)a + (long)b + (long)c;
      },
      left, right, third);
  if (triple != 18L) {
    return 6;
  }

  std::variant<int, long> fourth(8L);
  long quad = std::visit<long>(
      [](auto a, auto b, auto c, auto d) {
        return (long)a + (long)b + (long)c + (long)d;
      },
      left, right, third, fourth);
  if (quad != 26L) {
    return 7;
  }

  std::variant<int, long> fifth(9);
  long five = std::visit<long>(
      [](auto a, auto b, auto c, auto d, auto e) {
        return (long)a + (long)b + (long)c + (long)d + (long)e;
      },
      left, right, third, fourth, fifth);
  if (five != 35L) {
    return 8;
  }

  seen = 0;
  std::visit<void>(
      [&](auto a, auto b, auto c, auto d) {
        seen = (int)((long)a + (long)b + (long)c + (long)d);
      },
      left, right, third, fourth);
  if (seen != 26) {
    return 9;
  }

  return 0;
}
