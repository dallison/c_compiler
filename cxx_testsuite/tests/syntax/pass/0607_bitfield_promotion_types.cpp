// RUN: -std=c++20
// Integral promotion of bit-fields ([conv.prom]/5) in types and constant
// expressions: a field whose values all fit in int promotes to int, and an
// unsigned field as wide as int promotes to unsigned int.
template <class T, class U> struct Same { static constexpr bool value = false; };
template <class T> struct Same<T, T> { static constexpr bool value = true; };

struct S {
  unsigned c : 4;
  unsigned w : 32;
  unsigned long long l : 4;
  int s : 3;
};
constexpr S k{3, 3, 3, 1};
S g;

static_assert(k.c - 5 < 0);
static_assert(-k.c < 0);
static_assert(~k.c < 0);
static_assert(k.w - 5 > 0);
static_assert(k.l - 5 < 0);
static_assert(Same<decltype(k.c + 0u), unsigned>::value);
static_assert(Same<decltype(k.c + 0), int>::value);
static_assert(Same<decltype(-k.c), int>::value);
static_assert(Same<decltype(~k.w), unsigned>::value);
static_assert(Same<decltype(k.l << 1), int>::value);
static_assert(Same<decltype(g.c), unsigned>::value);
static_assert(Same<decltype(+g.c), int>::value);
