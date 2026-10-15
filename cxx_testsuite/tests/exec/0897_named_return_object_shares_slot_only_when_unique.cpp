// Only one object can be built in the return slot.  A local is constructed
// there only when every return that can run while it is alive returns it;
// otherwise two locals share the slot, or another value overwrites one.
static int live = 0;
static int moves = 0;
static int copies = 0;

struct S {
  int v;
  S(int x) : v(x) { ++live; }
  S(const S& o) : v(o.v) {
    ++live;
    ++copies;
  }
  S(S&& o) : v(o.v) {
    o.v = -1;
    ++live;
    ++moves;
  }
  ~S() { --live; }
};

S Two(int k) {
  S a(1);
  {
    S b(2);
    if (k) return b;
  }
  return a;
}

S Mixed(int k) {
  S a(1);
  if (k) return S(7);
  return a;
}

S Early(int k) {
  if (k) return S(9);
  S a(1);
  a.v = 5;
  return a;
}

S Loop(int k) {
  S a(1);
  for (int i = 0; i < 3; ++i) {
    S b(10 + i);
    if (k == i) return b;
  }
  return a;
}

S Cond(int k) {
  S a(1), b(2);
  return k ? a : b;
}

int main() {
  {
    S r = Two(0);
    S q = Two(1);
    if (r.v != 1 || q.v != 2) return 1;
  }
  {
    S r = Mixed(0);
    S q = Mixed(1);
    if (r.v != 1 || q.v != 7) return 2;
  }
  int moves_before = moves;
  int copies_before = copies;
  {
    S r = Early(0);
    S q = Early(1);
    if (r.v != 5 || q.v != 9) return 3;
  }
  // The only object Early returns by name is still built in the return slot.
  if (moves != moves_before || copies != copies_before) return 4;
  {
    S r = Loop(1);
    S q = Loop(5);
    if (r.v != 11 || q.v != 1) return 5;
  }
  {
    S r = Cond(1);
    S q = Cond(0);
    if (r.v != 1 || q.v != 2) return 6;
  }
  if (live != 0) return 7;
  return 0;
}
