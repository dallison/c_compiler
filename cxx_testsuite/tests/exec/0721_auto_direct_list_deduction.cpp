// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <initializer_list>
#include <limits>

static int kind(long) {
  return 1;
}

static int kind(std::initializer_list<long>) {
  return 2;
}

constexpr auto global_direct{7L};

template <typename T>
static int dependent_kind(T value) {
  auto direct{value};
  return kind(direct);
}

int main() {
  constexpr auto direct{(std::numeric_limits<long>::max)()};
  auto copy = {(std::numeric_limits<long>::max)()};
  return kind(global_direct) == 1 && kind(direct) == 1 && kind(copy) == 2 &&
                 dependent_kind(3L) == 1
             ? 0
             : 1;
}
