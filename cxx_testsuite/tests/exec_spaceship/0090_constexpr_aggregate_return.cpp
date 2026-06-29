// Returning a class/aggregate by value from a constexpr function and then
// reading a member of the (prvalue) result inside a constant expression.
// This exercises the constant-expression evaluator's support for:
//   * a free function returning a struct by value, then `.member`,
//   * a member function returning a struct by value, then `.member`,
//   * a single-member aggregate (whose `{x}` brace must not collapse to `x`),
//   * a member operator applied to a by-value temporary.
struct Point {
  int x;
  int y;
  constexpr Point shifted(int dx, int dy) const { return Point{x + dx, y + dy}; }
};

struct Boxed {
  int v;
  constexpr bool operator<(int n) const { return v < n; }
  constexpr bool operator==(int n) const { return v == n; }
};

constexpr Point make_point(int x, int y) { return Point{x, y}; }
constexpr Boxed box(int v) { return Boxed{v}; }

static_assert(make_point(3, 4).x == 3, "free aggregate return, member x");
static_assert(make_point(3, 4).y == 4, "free aggregate return, member y");
static_assert(make_point(3, 4).shifted(10, 20).x == 13, "chained member call");
static_assert(make_point(3, 4).shifted(10, 20).y == 24, "chained member call y");
static_assert(box(7) < 10, "member operator< on by-value result");
static_assert(box(5) == 5, "member operator== on by-value result");
static_assert(!(box(7) == 8), "member operator== false case");

int main() {
  if (make_point(3, 4).x != 3) return 1;
  if (make_point(3, 4).shifted(10, 20).x != 13) return 2;
  if (make_point(3, 4).shifted(10, 20).y != 24) return 3;
  if (!(box(7) < 10)) return 4;
  if (!(box(5) == 5)) return 5;
  return 0;
}
