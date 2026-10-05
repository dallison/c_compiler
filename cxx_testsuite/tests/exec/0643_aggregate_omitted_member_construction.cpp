// RUN: -std=c++20
// EXPECT_EXIT: 0

// [dcl.init.aggr]/5: an aggregate member (or base, or array element) that no
// initializer reaches is copy-initialized from `{}`, so a class member runs its
// default constructor and an aggregate element gets its default member
// initializers; the rest of the object is still zeroed.  Implicit destructors
// destroy every element of a multidimensional array member.
static int made = 0, gone = 0;
struct S {
  char* p;
  long n;
  char buf[16];
  S() : p(buf), n(7) { buf[0] = 0; ++made; }
  S(const S& o) : p(buf), n(o.n) { buf[0] = 0; ++made; }
  ~S() { ++gone; }
};
struct Named { S name; int k; };
struct Outer { int a; Named inner; };
struct WithArr { S arr[2]; int z; };
struct Base : S { int b; };
struct Pair { S first; S second; };
union U { int i; long l; };
struct HasU { U u; S s; };
struct A { int x = 5; int y; };
struct W { A arr[3]; int z; };
struct Grid { int q; S arr[2][2]; };
struct Inner { long a, b; };
struct Wrap { Inner i; long x; };
Named global_named{};

static long read_x(Wrap* w) { return w->x; }

int main() {
  {
    Named x{};
    if (x.name.p != x.name.buf || x.name.n != 7 || x.k != 0) return 1;
    Named y{S(), 3};
    if (y.k != 3 || y.name.p != y.name.buf) return 2;
    Outer o{5};
    if (o.inner.name.p != o.inner.name.buf || o.inner.k != 0 || o.a != 5)
      return 3;
    Base b{};
    if (b.p != b.buf || b.n != 7 || b.b != 0) return 4;
    WithArr w{};
    if (w.arr[1].p != w.arr[1].buf || w.arr[0].n != 7 || w.z != 0) return 5;
    Pair pr{S()};
    if (pr.second.p != pr.second.buf) return 6;
    HasU h{};
    if (h.s.n != 7 || h.u.l != 0) return 7;
    W aw{};
    if (aw.arr[0].x != 5 || aw.arr[2].x != 5 || aw.arr[1].y != 0) return 8;
    W aw2{{{1, 2}}};
    if (aw2.arr[0].x != 1 || aw2.arr[0].y != 2 || aw2.arr[1].x != 5) return 9;
    Grid g{1};
    if (g.arr[1][1].p != g.arr[1][1].buf || g.q != 1) return 10;
    Named* heap = new Named[2]{};
    if (heap[1].name.p != heap[1].name.buf || heap[1].k != 0) return 11;
    delete[] heap;
  }
  if (global_named.name.p != global_named.name.buf) return 12;
  // Every object made in the block is gone; the global is still alive.
  if (made != gone + 1) return 13;
  Inner in{1, 2};
  Wrap wrap{in};
  if (read_x(&wrap) != 0 || wrap.i.b != 2) return 14;
  return 0;
}
