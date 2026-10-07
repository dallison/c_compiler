// RUN: -std=c++20 -fconstexpr-eval=audit

// Only the functions that read the dynamic exception state are left unfolded
// while a function body is analyzed.  A function that merely has "exception"
// in its name, such as std::variant's valueless_by_exception, folds like any
// other, so both evaluators agree on calls made outside a constant context.

#include <variant>

struct Box {
  int index;
  constexpr Box() : index(-1) {}
  constexpr bool valueless_by_exception() const noexcept {
    return index == -1;
  }
  constexpr ~Box() {
    if (!valueless_by_exception()) {
      index = 0;
    }
  }
};

constexpr int UsesBox() {
  Box b;
  return b.valueless_by_exception() ? 1 : 0;
}

constexpr int UsesVariant() {
  std::variant<int, long> v = 3L;
  return v.valueless_by_exception() ? -1 : static_cast<int>(v.index());
}

int Runtime() {
  if (UsesBox() != 1) {
    return 1;
  }
  if (UsesVariant() != 1) {
    return 2;
  }
  return 0;
}

static_assert(UsesBox() == 1);
static_assert(UsesVariant() == 1);
