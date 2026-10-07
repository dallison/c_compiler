// RUN: -std=c++20
// EXPECT_EXIT: 0

// The constexpr std::variant operations give the same results whether they are
// constant-evaluated or run: construction of a later alternative, emplace,
// assignment that switches alternatives, and visit.

#include <variant>

struct P {
  int a;
  constexpr P(int x) : a(x) {}
};

struct Doubler {
  constexpr int operator()(int x) const { return 2 * x; }
  constexpr int operator()(const P& p) const { return 3 * p.a; }
};

constexpr int InPlace() {
  std::variant<int, P> v(std::in_place_index<1>, 6);
  return std::get<P>(v).a;
}

constexpr int Emplace() {
  std::variant<int, P> v;
  v.emplace<1>(8);
  return std::get<1>(v).a;
}

constexpr int Reassign() {
  std::variant<int, long> v = 3L;
  v = 4;
  return static_cast<int>(v.index()) * 10 + std::get<0>(v);
}

constexpr int Visit() {
  std::variant<int, P> a = 4;
  std::variant<int, P> b = P(5);
  return std::visit(Doubler{}, a) + std::visit(Doubler{}, b);
}

constexpr int kInPlace = InPlace();
constexpr int kEmplace = Emplace();
constexpr int kReassign = Reassign();
constexpr int kVisit = Visit();

int (*volatile in_place)() = InPlace;
int (*volatile emplace)() = Emplace;
int (*volatile reassign)() = Reassign;
int (*volatile visit)() = Visit;

int main() {
  if (kInPlace != 6 || InPlace() != 6 || in_place() != 6) {
    return 1;
  }
  if (kEmplace != 8 || Emplace() != 8 || emplace() != 8) {
    return 2;
  }
  if (kReassign != 4 || Reassign() != 4 || reassign() != 4) {
    return 3;
  }
  if (kVisit != 23 || Visit() != 23 || visit() != 23) {
    return 4;
  }
  return 0;
}
