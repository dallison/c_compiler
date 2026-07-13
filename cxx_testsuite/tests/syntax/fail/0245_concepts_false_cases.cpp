// RUN: -std=c++20
// EXPECT: static assertion failed
#include <concepts>

struct PublicBase {};
struct PrivateDerived : private PublicBase {};
struct Left : PublicBase {};
struct Right : PublicBase {};
struct AmbiguousDerived : Left, Right {};

struct ExplicitOnly {
  explicit ExplicitOnly(int) {}
};

// operator== yields a type that is not usable in a boolean context, so this is
// genuinely not equality_comparable. (A type returning `int` here would be
// equality_comparable, since `int` is boolean-testable.)
struct NotBoolean {};

struct NonBoolEq {
  int value;
  NotBoolean operator==(const NonBoolEq&) const { return NotBoolean{}; }
};

struct NonSwappable {
  NonSwappable() {}
  NonSwappable(const NonSwappable&) = delete;
  NonSwappable& operator=(const NonSwappable&) = delete;
};

static_assert(std::derived_from<PrivateDerived, PublicBase>,
              "static assertion failed");
static_assert(std::derived_from<AmbiguousDerived, PublicBase>,
              "static assertion failed");
static_assert(std::convertible_to<ExplicitOnly, int>,
              "static assertion failed");
static_assert(std::equality_comparable<NonBoolEq>,
              "static assertion failed");
static_assert(std::swappable<NonSwappable>,
              "static assertion failed");
static_assert(std::same_as<int, long>,
              "static assertion failed");

int main() {
  return 0;
}
