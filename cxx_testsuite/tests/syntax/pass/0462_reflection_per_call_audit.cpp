// RUN: -std=c++26 -fconstexpr-eval=audit

template <class T>
struct Box {};

constexpr auto reflected_box = ^^Box<int>;

constexpr int ordinary(int value) {
  return value + 1;
}

consteval int reflection_only() {
  constexpr auto reflected_int = ^^int;
  return reflected_int == ^^int ? 17 : 0;
}

static_assert(ordinary(41) == 42);
static_assert(reflection_only() == 17);
