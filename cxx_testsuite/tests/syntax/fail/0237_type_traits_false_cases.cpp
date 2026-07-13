// RUN: -std=c++20
// EXPECT: static assertion failed
#include <type_traits>

struct NoDefault {
  explicit NoDefault(int) {}
};

struct DeletedCopy {
  DeletedCopy() {}
  DeletedCopy(const DeletedCopy&) = delete;
};

struct ExplicitOnly {
  explicit ExplicitOnly(int) {}
};

struct NonSwappable {
  NonSwappable() {}
  NonSwappable(const NonSwappable&) = delete;
  NonSwappable& operator=(const NonSwappable&) = delete;
};

struct Base {
  int value;
};

struct Left : Base {};
struct Right : Base {};
struct Ambiguous : Left, Right {};

static_assert(std::is_default_constructible<NoDefault>::value,
              "static assertion failed");
static_assert(std::is_copy_constructible<DeletedCopy>::value,
              "static assertion failed");
static_assert(std::is_convertible<ExplicitOnly, int>::value,
              "static assertion failed");
static_assert(std::is_swappable<NonSwappable>::value,
              "static assertion failed");
static_assert(!std::is_base_of<Base, Ambiguous>::value,
              "static assertion failed");

int main() {
  return 0;
}
