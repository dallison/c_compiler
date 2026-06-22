// RUN: -std=c++20

struct Defaults {
  int x = 3;
  int y{4};
  int z = 7;

  constexpr Defaults(void) {}
  constexpr Defaults(int value) : y(value) {}
};

constexpr Defaults a{};
constexpr Defaults b(10);

static_assert(a.x == 3, "default member initializer with equals");
static_assert(a.y == 4, "default member initializer with braces");
static_assert(a.z == 7, "default member initializer can reference prior member");
static_assert(b.x == 3, "default member initializer fills omitted member");
static_assert(b.y == 10, "constructor initializer overrides default member initializer");
static_assert(b.z == 7, "default member initializer applies with explicit members");
