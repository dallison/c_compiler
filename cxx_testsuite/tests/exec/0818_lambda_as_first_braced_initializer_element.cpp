// A `[` that starts an element of a braced initializer is a lambda in C++, not
// an array designator.
#include <functional>

template <typename... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

struct Pair {
  int (*first)(int);
  int (*second)(int);
};

int main() {
  int base = 40;
  auto visitor = Overloaded{[](int v) { return v + 1; },
                            [&base](double) { return base; },
                            [=](const char*) mutable { return base + 2; }};
  if (visitor(1) != 2) return 1;
  if (visitor(1.5) != 40) return 2;
  if (visitor("x") != 42) return 3;

  std::function<int(int)> table[] = {[](int v) { return v * 2; },
                                     [base](int v) { return v + base; }};
  if (table[0](4) != 8) return 4;
  if (table[1](2) != 42) return 5;

  Pair p = {[](int v) { return v - 1; }, [](int v) { return -v; }};
  if (p.first(5) != 4 || p.second(5) != -5) return 6;

  int designated[4] = {[2] = 7, [0] = 1};
  if (designated[0] != 1 || designated[2] != 7 || designated[3] != 0) return 7;
  return 0;
}
