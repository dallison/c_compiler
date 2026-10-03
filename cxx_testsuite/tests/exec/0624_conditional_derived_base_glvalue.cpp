// RUN: -std=c++20
// EXPECT_EXIT: 0

// [expr.cond]/4: `c ? derived : base` with lvalue arms converts the derived
// arm to the base subobject and yields an lvalue of the base, which is what
// common_reference_t<D&, B&> is built from.

#include <type_traits>

struct A {
  int a = 1;
};
struct B {
  int b = 2;
};
struct D : A, B {
  int d = 3;
};

static_assert(std::is_same_v<decltype(true ? std::declval<D&>()
                                           : std::declval<B&>()),
                             B&>);
static_assert(std::is_same_v<decltype(true ? std::declval<const B&>()
                                           : std::declval<const D&>()),
                             const B&>);
static_assert(std::is_same_v<std::common_reference_t<D&, B&>, B&>);
static_assert(std::is_same_v<std::common_reference_t<D&&, const B&>, const B&>);
static_assert(std::is_same_v<std::common_reference_t<const int&, const long&>,
                             long>);
static_assert(std::is_same_v<std::common_reference_t<int&&, int&>, const int&>);
static_assert(std::is_same_v<std::common_reference_t<int&, int>, int>);

int pick(bool c, D& d, B& b) { return (c ? d : b).b; }

int& select(bool c, D& d, B& b) { return (c ? d : b).b; }

int main() {
  D d;
  B b;
  b.b = 7;
  d.b = 5;
  if (pick(true, d, b) != 5 || pick(false, d, b) != 7) {
    return 1;
  }
  select(true, d, b) = 9;
  if (d.b != 9 || b.b != 7) {
    return 2;
  }
  const B& chosen = true ? d : b;
  if (&chosen != static_cast<B*>(&d)) {
    return 3;
  }
  return 0;
}
