// RUN: -std=c++23 -fconstexpr-eval=pcode

// Before C++26 std::construct_at is the one place a constant expression may
// use placement new, so the library's constexpr in-place construction goes
// through it.
#include <expected>
#include <memory>
#include <variant>

struct Point {
  int x, y;
  constexpr Point(int a, int b) : x(a), y(b) {}
};

constexpr bool construct_at_forwards_arguments() {
  union Slot {
    Point point;
    char none;
    constexpr Slot() : none(0) {}
  } slot;
  Point* point = std::construct_at(&slot.point, 3, 4);
  bool ok = point == &slot.point && point->x + point->y == 7;
  std::destroy_at(point);
  return ok;
}
static_assert(construct_at_forwards_arguments());

constexpr bool variant_copies_in_place() {
  std::variant<int, Point> original(std::in_place_index<1>, 1, 2);
  std::variant<int, Point> copy = original;
  copy = std::variant<int, Point>(std::in_place_index<0>, 9);
  return std::get<1>(original).y == 2 && std::get<0>(copy) == 9;
}
static_assert(variant_copies_in_place());

constexpr bool expected_constructs_in_place() {
  std::expected<int, Point> failed(std::unexpect, 5, 6);
  std::expected<int, Point> copy = failed;
  std::expected<int, Point> value(7);
  return copy.error().x == 5 && *value == 7;
}
static_assert(expected_constructs_in_place());
