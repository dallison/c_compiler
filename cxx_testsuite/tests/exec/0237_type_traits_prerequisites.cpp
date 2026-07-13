// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <functional>
#include <type_traits>
#include <utility>

struct IntConstruct {
  int value;
  explicit IntConstruct(int initial) : value(initial) {}
};

struct DeletedCopy {
  DeletedCopy() {}
  DeletedCopy(const DeletedCopy&) = delete;
  DeletedCopy& operator=(const DeletedCopy&) = delete;
};

struct NonSwappable {
  NonSwappable() {}
  NonSwappable(const NonSwappable&) = delete;
  NonSwappable& operator=(const NonSwappable&) = delete;
};

struct PublicBase {
  int value;
};

struct PublicDerived : PublicBase {
  int extra;
};

struct Callable {
  int operator()(int value) const { return value + 5; }
};

int main() {
  if (std::is_default_constructible<IntConstruct>::value) {
    return 1;
  }
  if (std::is_copy_constructible<DeletedCopy>::value) {
    return 2;
  }
  if (std::is_convertible<IntConstruct, int>::value) {
    return 3;
  }
  if (std::is_swappable<NonSwappable>::value) {
    return 4;
  }
  if (!std::is_base_of<PublicBase, PublicDerived>::value) {
    return 5;
  }
  if (!std::is_swappable<int>::value) {
    return 6;
  }

  int value = 9;
  std::reference_wrapper<const int> wrapped = std::cref(value);
  if (wrapped.get() != 9) {
    return 7;
  }

  Callable callable;
  if (std::invoke(callable, 10) != 15) {
    return 8;
  }

  if (!std::is_invocable<Callable, int>::value) {
    return 9;
  }

  return 0;
}
