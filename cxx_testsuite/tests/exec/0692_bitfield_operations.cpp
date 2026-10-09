// Bit-fields that share a storage unit: braced initialization, compound
// assignment, increment and decrement, the value of an assignment, fields
// wider than 32 bits, constructor and default member initializers, static
// data for constant and dynamically initialized globals, and folding.
#define NOINLINE __attribute__((noinline))

struct B { int a : 3; int b : 5; unsigned c : 4; };

NOINLINE int Init(int x) {
  B r{1, x, 9};
  return r.a * 10000 + r.b * 100 + (int)r.c;
}
NOINLINE int Compound(int x) {
  B r{1, 2, 9};
  r.b += x;
  return r.a * 10000 + r.b * 100 + (int)r.c;
}
NOINLINE int AssignValue(int x) {
  B r{1, 2, 9};
  int v = (r.b = x);
  return v * 10000 + r.a * 100 + (int)r.c;
}
NOINLINE int CompoundValue(int x) {
  B r{1, 2, 9};
  int v = (r.b += x);
  return v * 10000 + r.a * 100 + (int)r.c;
}
NOINLINE int PreInc(int x) {
  B r{1, x, 9};
  int v = ++r.b;
  return v * 10000 + r.a * 100 + (int)r.c;
}
NOINLINE int PostInc(int x) {
  B r{1, x, 9};
  int v = r.b++;
  return v * 10000 + r.b * 100 + r.a * 10 + (int)r.c;
}
NOINLINE int PostDec(int x) {
  B r{1, x, 9};
  int v = r.b--;
  return v * 10000 + r.b * 100 + r.a * 10 + (int)r.c;
}

struct W { long long s : 40; unsigned long long u : 36; int t : 4; };
NOINLINE long long WideS(long long x) {
  W w{x, 0, 1};
  return w.s;
}
NOINLINE unsigned long long WideU(unsigned long long x) {
  W w{};
  w.u = x;
  w.t = -1;
  return w.u;
}
NOINLINE long long WideInc(long long x) {
  W w{x, 5, 2};
  w.s++;
  return w.s + w.t;
}

struct M { int x : 4; int y : 4; M(int v) : x(v), y(v + 1) {} };
struct D { int a : 3 = 7; unsigned b : 2 = 5; int c : 5 = 9; };
union U { struct { unsigned lo : 4, hi : 4; } s; unsigned char c; };
NOINLINE int UnionInit(unsigned x) {
  U u{{x, 3}};
  return u.c;
}

NOINLINE int Get() { return 17; }
B dynamic_global{-1, Get(), 9};

constexpr B MakeB(int x) {
  B r{};
  r.a = x;
  r.b = x * 2;
  r.c = x;
  return r;
}
constexpr B constant_global{-1, 2, 3};
constexpr B computed_global = MakeB(-2);
constexpr W wide_global{-(1LL << 39), (1ULL << 35) + 3, -2};
NOINLINE int ReadVia(const B* p) {
  return p->a * 10000 + p->b * 100 + (int)p->c;
}
NOINLINE int Folded() { return B{-1, 2, 3}.b; }

int main() {
  if (Init(-16) != 10000 - 1600 + 9) return 1;
  if (Compound(14) != 10000 - 1600 + 9) return 2;
  if (AssignValue(17) != -150000 + 100 + 9) return 3;
  if (CompoundValue(14) != -160000 + 100 + 9) return 4;
  if (PreInc(15) != -160000 + 100 + 9) return 5;
  if (PostInc(15) != 150000 - 1600 + 10 + 9) return 6;
  if (PostDec(-16) != -160000 + 1500 + 10 + 9) return 7;
  if (WideS(-(1LL << 39)) != -(1LL << 39)) return 8;
  if (WideS(1LL << 39) != -(1LL << 39)) return 9;
  if (WideU((1ULL << 36) + 5) != 5) return 10;
  if (WideInc((1LL << 39) - 1) != -(1LL << 39) + 2) return 11;
  M m(7);
  if (m.x != 7 || m.y != -8) return 12;
  D d;
  if (d.a != -1 || d.b != 1 || d.c != 9) return 13;
  if (UnionInit(0x1a) != 0x3a) return 14;
  if (dynamic_global.a != -1 || dynamic_global.b != -15 ||
      dynamic_global.c != 9) {
    return 15;
  }
  if (ReadVia(&constant_global) != -10000 + 200 + 3) return 16;
  if (ReadVia(&computed_global) != -20000 - 400 + 14) return 17;
  const W* volatile wide = &wide_global;
  if (wide->s != -(1LL << 39) || wide->u != (1ULL << 35) + 3 ||
      wide->t != -2) {
    return 18;
  }
  if (Folded() != 2) return 19;
  return 0;
}
