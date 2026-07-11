// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <utility>

struct Value {
  int value;

  Value() : value(0) {}
  Value(int v) : value(v) {}
};

template <class T>
struct Box {
  using mapped_type = T;

  template <class... Args>
  mapped_type make(Args&&... args) {
    return mapped_type(std::forward<Args>(args)...);
  }
};

int main() {
  Box<Value> box;
  Value value = box.make(42);
  return value.value == 42 ? 0 : 1;
}
