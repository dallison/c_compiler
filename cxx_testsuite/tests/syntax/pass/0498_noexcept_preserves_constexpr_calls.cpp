// RUN: -std=c++20

constexpr int potentially_throwing() noexcept(false) {
  return 1;
}

constexpr int nonthrowing() noexcept {
  return 1;
}

struct callable {
  constexpr int throwing_member() const noexcept(false) { return 1; }
  constexpr int nonthrowing_member() const noexcept { return 1; }
};

constexpr callable object{};

static_assert(!noexcept(potentially_throwing()));
static_assert(noexcept(nonthrowing()));
static_assert(!noexcept(object.throwing_member()));
static_assert(noexcept(object.nonthrowing_member()));
static_assert(noexcept(noexcept(potentially_throwing())));
static_assert(noexcept(sizeof(potentially_throwing())));
