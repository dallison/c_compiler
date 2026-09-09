// RUN: -std=c++20 -fconstexpr-eval=pcode

struct Box {
  int value;

  constexpr Box(int v, ...) : value(v) {}
};

constexpr int make_box() {
  Box box(41, 1, 2, 3);
  return box.value;
}

constexpr Box global_box(40, 8, 9);

static_assert(make_box() == 41);
static_assert(global_box.value == 40);
static_assert(Box(42, 1, 2).value == 42);
