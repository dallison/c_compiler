// RUN: -std=c++20
// EXPECT_EXIT: 0
// Namespace-scope aggregates initialized by braces ([dcl.init.aggr]) are not
// default-constructed afterwards, so explicit values survive; a member the
// initializer omits whose constructor is not constexpr is constructed by
// dynamic initialization ([basic.start.dynamic]), including for arrays, bases
// and thread_local objects.

struct N { int v; N() : v(42) {} };
struct A { int x; N n; };
struct A2 { int x = 3; N n; };
struct B : A { int y; };
struct Inner { int p; int q = 7; };
struct Outer { Inner in; int z; int arr[2]; };
struct K { Inner in; int z; ~K() {} };
A a = {1};
A2 a2 = {1};
A arr[2] = {{1}};
B b = {{5}, 6};
thread_local A ta = {1};
thread_local Outer tg = {1, 2, 3, 4};
Outer g = {1, 2, 3, 4};
static Outer sg = {1, 2, 3, 4};
K k = {1, 2};
int main() {
  if (a.x != 1 || a.n.v != 42) return 1;
  if (a2.x != 1 || a2.n.v != 42) return 2;
  if (arr[0].x != 1 || arr[0].n.v != 42 || arr[1].n.v != 42) return 3;
  if (b.x != 5 || b.n.v != 42 || b.y != 6) return 4;
  if (ta.x != 1 || ta.n.v != 42) return 5;
  if (tg.in.q != 2 || tg.z != 3) return 6;
  if (g.in.q != 2 || sg.arr[0] != 4) return 7;
  if (k.in.q != 2 || k.z != 0) return 8;
  static Outer lg = {1, 2, 3, 4};
  static A la = {1};
  if (lg.in.q != 2 || la.n.v != 42) return 9;
  return 0;
}
