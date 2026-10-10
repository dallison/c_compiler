// Constant evaluation calls through function pointers and references: a
// constexpr pointer or reference variable, a parameter, a member of a
// constexpr aggregate, and a dereferenced pointer all designate the function
// they hold.
constexpr int Four() { return 4; }
constexpr int Twice(int x) { return 2 * x; }

constexpr int (*pointer)() = Four;
constexpr int (&reference)() = Four;
static_assert(pointer() == 4);
static_assert(reference() == 4);
static_assert((*pointer)() == 4);
static_assert(&reference == &Four);

struct Ops {
  int (*unary)(int);
  int (&nullary)();
};
constexpr Ops ops{Twice, Four};
static_assert(ops.unary(5) == 10);
static_assert(ops.nullary() == 4);
static_assert((*ops.unary)(3) == 6);

constexpr int CallPointer(int (*f)(int), int x) { return f(x); }
constexpr int CallReference(int (&f)()) { return f() + 1; }
constexpr int CallMember(const Ops& o) { return o.unary(o.nullary()); }
static_assert(CallPointer(Twice, 7) == 14);
static_assert(CallReference(Four) == 5);
static_assert(CallMember(ops) == 8);

constexpr int Local() {
  int (*f)(int) = Twice;
  int (&g)() = Four;
  Ops local{Twice, Four};
  return f(g()) + local.unary(1) + (*local.unary)(2);
}
static_assert(Local() == 14);

int main() {
  if (pointer() != 4 || reference() != 4) return 1;
  if (ops.unary(6) != 12 || ops.nullary() != 4) return 2;
  if (CallMember(ops) != 8 || Local() != 14) return 3;
  return 0;
}
