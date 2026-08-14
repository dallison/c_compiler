// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <new>

struct state {
  unsigned value;
};

struct holder {
  state current;

  constexpr holder() : current() {}
};

int main() {
  unsigned storage = ~0u;
  holder* value = new (&storage) holder;
  return value->current.value == 0 ? 0 : 1;
}
