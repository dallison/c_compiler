// RUN: -std=c++26 -fconstexpr-eval=pcode
// EXPECT_EXIT: 0

// The p-code constant evaluator running for a real target: exception_ptr
// handles in 32-bit pointers, virtual calls through a vtable whose type_info
// lives in the C++ runtime, and floating-point exceptions bound to a handler
// parameter.
#include <exception>

struct V {
  int k;
  constexpr V(int x) : k(x) {}
  constexpr virtual int get() const { return k; }
};

struct W : V {
  constexpr W() : V(4) {}
  constexpr int get() const override { return 9; }
};

constexpr int virtual_call() {
  W w;
  const V& v = w;
  return v.get();
}

constexpr int caught_virtual_call() {
  try {
    throw W();
  } catch (const V& v) {
    return v.get();
  }
  return 0;
}

constexpr int caught_double() {
  try {
    throw 3.5;
  } catch (int) {
    return -1;
  } catch (double v) {
    return v == 3.5 ? 35 : -2;
  }
  return 0;
}

constexpr int caught_float() {
  try {
    throw 2.5f;
  } catch (float v) {
    return v == 2.5f ? 25 : -1;
  }
  return 0;
}

constexpr int captured_exception() {
  std::exception_ptr e;
  try {
    throw 31;
  } catch (...) {
    e = std::current_exception();
  }
  if (e == nullptr) {
    return -1;
  }
  try {
    std::rethrow_exception(e);
  } catch (int v) {
    return v;
  }
  return 0;
}

constexpr int copied_exception_ptr() {
  try {
    throw 17;
  } catch (...) {
    std::exception_ptr first = std::current_exception();
    std::exception_ptr second = first;
    first = nullptr;
    try {
      std::rethrow_exception(second);
    } catch (int v) {
      return v;
    }
  }
  return 0;
}

static_assert(virtual_call() == 9);
static_assert(caught_virtual_call() == 9);
static_assert(caught_double() == 35);
static_assert(caught_float() == 25);
static_assert(captured_exception() == 31);
static_assert(copied_exception_ptr() == 17);

int main() {
  if (virtual_call() != 9) return 1;
  if (caught_virtual_call() != 9) return 2;
  if (caught_double() != 35) return 3;
  if (caught_float() != 25) return 4;
  if (captured_exception() != 31) return 5;
  if (copied_exception_ptr() != 17) return 6;
  return 0;
}
