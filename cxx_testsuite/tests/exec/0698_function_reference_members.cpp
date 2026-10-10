// A reference-to-function member designates its function: a call through it
// calls that function, in plain and constexpr aggregates alike.  `*f` names
// the function f itself, however many times it is applied.
int Three() { return 3; }
int Four() { return 4; }

struct FR {
  int (&fn)();
  int d;
};
struct Holder {
  int (&fn)();
  constexpr Holder(int (&f)()) : fn(f) {}
};

FR global{Three, 1};
constexpr FR constant{Four, 2};
constexpr Holder held{Three};
static_assert(&constant.fn == &Four);
static_assert(&held.fn == &Three);

int CallThrough(FR& r) { return r.fn(); }
int CallConst(const FR& r) { return r.fn() + r.d; }

int main() {
  if (global.fn() != 3) return 1;
  if (constant.fn() != 4) return 2;
  if (held.fn() != 3) return 3;
  FR local{Four, 5};
  if (CallThrough(local) != 4) return 4;
  if (CallConst(constant) != 6) return 5;
  int (&alias)() = local.fn;
  if (alias() != 4) return 6;
  if ((*Three)() != 3 || (**Four)() != 4) return 7;
  int (*pointer)() = Three;
  if ((***pointer)() != 3 || (&*Four)() != 4) return 8;
  if ((*global.fn)() != 3) return 9;
  return 0;
}
