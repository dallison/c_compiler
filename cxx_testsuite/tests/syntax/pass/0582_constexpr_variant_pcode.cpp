// RUN: -std=c++20 -fconstexpr-eval=pcode

// 0581 with the p-code evaluator: std::variant in constant evaluation.

#include <variant>

struct P {
  int a;
  constexpr P(int x) : a(x) {}
};

constexpr int ScalarAlternative() {
  std::variant<int, long> v = 3L;
  return v.index() == 1 ? static_cast<int>(std::get<1>(v)) : -1;
}

constexpr int ClassAlternative() {
  std::variant<int, P> v(P(5));
  return std::get<1>(v).a;
}

constexpr int InPlace() {
  std::variant<int, P> v(std::in_place_index<1>, 6);
  return std::get<P>(v).a;
}

constexpr int Reassign() {
  std::variant<int, long> v = 3L;
  v = 4;
  return static_cast<int>(v.index()) * 10 + std::get<0>(v);
}

constexpr int Emplace() {
  std::variant<int, P> v;
  v.emplace<1>(8);
  return std::get<1>(v).a;
}

struct Doubler {
  constexpr int operator()(int x) const { return 2 * x; }
  constexpr int operator()(const P& p) const { return 3 * p.a; }
};

constexpr int Visit() {
  std::variant<int, P> a = 4;
  std::variant<int, P> b = P(5);
  return std::visit(Doubler{}, a) + std::visit(Doubler{}, b);
}

static_assert(ScalarAlternative() == 3);
static_assert(ClassAlternative() == 5);
static_assert(InPlace() == 6);
static_assert(Reassign() == 4);
static_assert(Emplace() == 8);
static_assert(Visit() == 23);
