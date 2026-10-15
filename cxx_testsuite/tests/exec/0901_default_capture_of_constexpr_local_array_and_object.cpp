// A default capture of a constexpr local array or class object captures the
// object itself; the lambda body must not read the enclosing frame's slot.
#include <string.h>

struct Point {
  int a, b;
};

static size_t Length(const char* s) { return strlen(s); }

constexpr int Evaluate() {
  constexpr int values[] = {1, 2, 3};
  constexpr Point point{4, 5};
  auto by_ref = [&]() { return values[1] + point.b; };
  auto by_copy = [=]() { return values[2] + point.a; };
  return by_ref() + by_copy();
}
static_assert(Evaluate() == 14, "");

int main() {
  constexpr char channel[] = "stuck_warning";
  auto by_ref = [&]() { return Length(channel); };
  auto by_copy = [=]() { return Length(channel); };
  if (by_ref() != 13 || by_copy() != 13) return 1;

  constexpr Point point{5, 6};
  const Point* seen = nullptr;
  auto members = [&]() { return point.a + point.b; };
  auto address = [&]() { seen = &point; };
  address();
  if (members() != 11 || seen != &point) return 2;

  constexpr int count = 7;
  auto scalar = [&]() { return count; };
  if (scalar() != 7) return 3;
  return Evaluate() == 14 ? 0 : 4;
}
