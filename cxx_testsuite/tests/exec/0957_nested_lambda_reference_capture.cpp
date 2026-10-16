// A lambda that captures by reference a variable which its enclosing lambda
// also captures by reference stores the address of the variable, reached
// through the enclosing closure, not the variable's value.
struct Visitor {
  template <typename F> void VisitAll(F f) { f(4); }
  int Run() {
    int a = -1, b = -1;
    VisitAll([&](auto key) {
      auto l = [&](int p) { return p + a * 0 + 1; };
      b = l(key);
      a = 2;
    });
    return a * 16 + b;
  }
};

int main() {
  int a = 5;

  auto plain = [&](int key) {
    auto l = [&](int p) { return p + a; };
    return l(key);
  };
  if (plain(1) != 6) return 1;

  auto generic_outer = [&](auto key) {
    auto l = [&](int p) { return p + a; };
    return l(key);
  };
  if (generic_outer(1) != 6) return 2;

  auto explicit_capture = [&](auto key) {
    auto l = [&a](int p) { return p + a; };
    return l(key);
  };
  if (explicit_capture(1) != 6) return 3;

  auto both_generic = [&](auto key) {
    auto l = [&](auto p) { return p + a; };
    return l(key);
  };
  if (both_generic(1) != 6) return 4;

  auto writes = [&](int v) {
    auto set = [&](int p) { a = p; };
    set(v);
  };
  writes(9);
  if (a != 9) return 5;

  Visitor visitor;
  if (visitor.Run() != 2 * 16 + 5) return 6;
  return 0;
}
