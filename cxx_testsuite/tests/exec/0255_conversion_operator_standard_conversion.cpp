// A user-defined conversion operator may be followed by a standard conversion
// to reach the requested type (a user-defined conversion sequence is
// [conversion operator][standard conversion]).  Also exercises best-candidate
// selection when a class has several conversion operators, and the contextual
// bool conversion through a non-bool operator.

struct HasLong {
  long v;
  operator long() const { return v; }
};

struct HasInt {
  int v;
  operator int() const { return v; }
};

struct TwoOps {
  // An exact match is preferred over a converting candidate.
  operator int() const { return 1; }
  operator long() const { return 100; }
};

struct Expl {
  int v;
  explicit operator int() const { return v; }
};

static int take_int(int x) { return x; }
static double take_double(double d) { return d; }

int main() {
  // operator long() then long -> int standard conversion.
  HasLong hl{42};
  int a = hl;
  if (a != 42) return 1;
  if (take_int(hl) != 42) return 2;

  // operator int() then int -> double.
  HasInt hi{7};
  double d = hi;
  if (d != 7.0) return 3;
  if (take_double(hi) != 7.0) return 4;

  // Contextual bool through operator int(): int -> bool.
  HasInt zero{0};
  HasInt nonzero{5};
  if (zero) return 5;
  if (!nonzero) return 6;
  int viaTernary = nonzero ? 11 : 22;
  if (viaTernary != 11) return 7;

  // Best-candidate selection: exact wins over converting.
  TwoOps t;
  int exact_int = t;      // prefers operator int() -> 1
  if (exact_int != 1) return 8;
  long exact_long = t;    // prefers operator long() -> 100
  if (exact_long != 100) return 9;
  // No exact operator for double: operator int()/operator long() both convert;
  // operator long() -> double and operator int() -> double are both
  // conversions, so this would be ambiguous -- do not test double here.

  // explicit operator int(): allowed in explicit cast and contextual bool,
  // including the int -> bool step for contextual bool.
  Expl e{9};
  if (static_cast<int>(e) != 9) return 10;
  if (!e) return 11;             // e -> int(9) -> bool(true)
  Expl ez{0};
  if (ez) return 12;             // e -> int(0) -> bool(false)

  return 0;
}
