// RUN: -std=c++23 -fconstexpr-eval=pcode
// EXPECT: placement new is not permitted in this constant expression

// Before C++26 a constant expression may only place an object through
// std::construct_at, however deep in the call chain the placement new is.
#include <new>

struct Point {
  int x, y;
};

constexpr Point* construct_in(Point* where, int x, int y) {
  return new (static_cast<void*>(where)) Point{x, y};
}

constexpr int placed() {
  Point point{0, 0};
  return construct_in(&point, 3, 4)->y;
}

static_assert(placed() == 4);
