// RUN: -std=c++26
// EXPECT: read of object outside its lifetime in constant expression

#include <memory>
#include <new>

struct box {
  int value;
  constexpr box(int v) : value(v) {}
  constexpr ~box() {}
};

constexpr int read_after_destroy() {
  box value = {42};
  std::destroy_at(&value);
  int result = value.value;
  new (static_cast<void*>(&value)) box{0};
  return result;
}

static_assert(read_after_destroy() == 42);
