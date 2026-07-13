// RUN: -std=c++20
#include <concepts>

struct EqOnly {
  int value;
  bool operator==(const EqOnly& other) const { return value == other.value; }
  bool operator!=(const EqOnly& other) const { return !(*this == other); }
};

struct Ordered {
  int value;
  bool operator==(const Ordered& other) const { return value == other.value; }
  bool operator!=(const Ordered& other) const { return !(*this == other); }
  bool operator<(const Ordered& other) const { return value < other.value; }
  bool operator>(const Ordered& other) const { return value > other.value; }
  bool operator<=(const Ordered& other) const { return value <= other.value; }
  bool operator>=(const Ordered& other) const { return value >= other.value; }
};

struct BoolLike {
  int value;
  // Implicit (non-explicit) conversion: convertible_to requires an *implicit*
  // conversion, so an explicit operator bool would not satisfy it.
  operator bool() const { return value != 0; }
  bool operator==(const BoolLike& other) const { return value == other.value; }
  bool operator!=(const BoolLike& other) const { return !(*this == other); }
};

// A comparison result that cannot be used in a boolean context. Because the
// standard's equality_comparable is built on boolean-testable, a type whose
// operator== yields a non-boolean-testable type is NOT equality_comparable.
// (Note: a type returning `int` from operator== *is* equality_comparable,
// since `int` is boolean-testable.)
struct NotBoolean {};

struct NonBoolEq {
  int value;
  NotBoolean operator==(const NonBoolEq&) const { return NotBoolean{}; }
  NotBoolean operator!=(const NonBoolEq&) const { return NotBoolean{}; }
};

struct LeftCmp {
  int value;
  bool operator==(const LeftCmp& other) const { return value == other.value; }
  bool operator!=(const LeftCmp& other) const { return !(*this == other); }
};

static_assert(std::equality_comparable<int>);
static_assert(std::equality_comparable<EqOnly>);
static_assert(!std::equality_comparable<NonBoolEq>);

static_assert(std::equality_comparable_with<int, long>);
static_assert(std::equality_comparable_with<LeftCmp, LeftCmp>);

static_assert(std::totally_ordered<int>);
static_assert(std::totally_ordered<Ordered>);
static_assert(!std::totally_ordered<EqOnly>);

static_assert(std::totally_ordered_with<int, long>);
static_assert(!std::totally_ordered_with<EqOnly, Ordered>);

static_assert(std::convertible_to<BoolLike, bool>);

int main() {
  return 0;
}
