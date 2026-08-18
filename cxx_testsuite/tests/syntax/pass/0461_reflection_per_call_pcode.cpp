// RUN: -std=c++26 -fconstexpr-eval=pcode

template <class T>
struct Box {};

constexpr auto reflected_box = ^^Box<int>;

constexpr int ordinary(int value) {
  return value * 2;
}

consteval int reflection_only() {
  constexpr auto reflected_int = ^^int;
  return reflected_int == ^^int ? sizeof(int) : 0;
}

static_assert(ordinary(21) == 42);
static_assert(reflection_only() == sizeof(int));
