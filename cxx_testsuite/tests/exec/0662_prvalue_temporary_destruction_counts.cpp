// RUN: -std=c++26
// EXPECT_EXIT: 0

// A prvalue initializes its result object directly, so each of these forms
// creates exactly one object that is destroyed exactly once -- whether the
// result object is a variable, a return slot, a parameter, or an exception
// object, and whether the prvalue is `T{...}`, `T(...)`, a call, or a
// conditional of those.

struct agg {
  int value;
  int* destroyed;
  constexpr ~agg() { *destroyed += 1; }
};

struct ctor {
  int value;
  int* destroyed;
  constexpr ctor(int v, int* d) : value(v), destroyed(d) {}
  constexpr ~ctor() { *destroyed += 1; }
};

constexpr agg make_agg(int v, int* d) { return agg{v, d}; }
constexpr ctor make_ctor(int v, int* d) { return ctor(v, d); }
constexpr agg pick_agg(bool w, int* d) { return w ? agg{1, d} : agg{2, d}; }
constexpr int take_agg(agg a) { return a.value; }
constexpr int take_ctor(ctor c) { return c.value; }

// Returns the number of destructor calls, or 100 + case on a wrong value.
constexpr int variable(int which) {
  int d = 0;
  {
    int v = 0;
    switch (which) {
      case 0: { agg a = agg{1, &d}; v = a.value; break; }
      case 1: { agg a = make_agg(1, &d); v = a.value; break; }
      case 2: { ctor c = make_ctor(1, &d); v = c.value; break; }
      case 3: { agg a = which > 9 ? agg{2, &d} : agg{1, &d}; v = a.value; break; }
      case 4: { ctor c = which > 9 ? ctor(2, &d) : ctor(1, &d); v = c.value; break; }
      case 5: { agg a = pick_agg(false, &d); v = a.value - 1; break; }
      case 6: v = take_agg(agg{1, &d}); break;
      case 7: v = take_ctor(ctor(1, &d)); break;
      case 8: v = take_agg(which > 9 ? agg{2, &d} : agg{1, &d}); break;
      case 9: v = (which > 9 ? agg{2, &d} : agg{1, &d}).value; break;
    }
    if (v != 1) return 100 + which;
  }
  return d;
}

constexpr int thrown(int which) {
  int d = 0;
  try {
    switch (which) {
      case 0: throw agg{1, &d};
      case 1: throw make_agg(1, &d);
      case 2: throw ctor(1, &d);
      case 3: throw make_ctor(1, &d);
      case 4: throw which > 9 ? agg{2, &d} : agg{1, &d};
      case 5: throw which > 9 ? ctor(2, &d) : ctor(1, &d);
    }
  } catch (const agg& a) {
    if (a.value != 1) return 100 + which;
  } catch (const ctor& c) {
    if (c.value != 1) return 100 + which;
  }
  return d;
}

constexpr bool all_once() {
  for (int i = 0; i <= 9; i++) {
    if (variable(i) != 1) return false;
  }
  for (int i = 0; i <= 5; i++) {
    if (thrown(i) != 1) return false;
  }
  return true;
}

static_assert(all_once());

int main() {
  for (int i = 0; i <= 9; i++) {
    if (variable(i) != 1) return 1 + i;
  }
  for (int i = 0; i <= 5; i++) {
    if (thrown(i) != 1) return 20 + i;
  }
  return 0;
}
