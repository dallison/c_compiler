// RUN: -std=c++23 -fconstexpr-eval=audit
// EXPECT: placement new is not permitted in this constant expression

#include <new>

constexpr int replace_before_cxx26() {
  int value = 1;
  new (static_cast<void*>(&value)) int(42);
  return value;
}

static_assert(replace_before_cxx26() == 42);
