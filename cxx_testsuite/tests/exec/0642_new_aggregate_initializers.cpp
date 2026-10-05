// RUN: -std=c++20
// EXPECT_EXIT: 0

// new-expression initializers: aggregate copy and slicing, parenthesized and
// designated aggregate init, array new with braced or parenthesized element
// lists (including deduced and multidimensional bounds), and constexpr new.
// Class objects and elements are constructed in place: the storage holds no
// object yet, so assigning into it would be wrong.
#include <string>
struct C { int value; };
struct D { int a; long b; C c; };
struct B : C { int extra; };
struct Named { std::string name; int n; };
int live = 0;
struct S {
  int v;
  S() : v(-1) { ++live; }
  S(int x) : v(x) { ++live; }
  S(int x, int y) : v(x * 10 + y) { ++live; }
  S(const S& o) : v(o.v) { ++live; }
  ~S() { --live; }
};
int assignments = 0;
struct Tracked {
  Tracked* self;
  Tracked() : self(this) {}
  Tracked(const Tracked&) : self(this) {}
  Tracked& operator=(const Tracked&) { ++assignments; return *this; }
};
struct Holder { Tracked t; int k; };
constexpr int constexpr_aggregate() {
  C* p = new C{40};
  C* q = new C(*p);
  int r = p->value + q->value / 20;
  delete p; delete q;
  return r;
}
static_assert(constexpr_aggregate() == 42);
int main() {
  C original{9};
  C* copy = new C{original};
  if (copy->value != 9) return 1;
  C* paren_copy = new C(original);
  if (paren_copy->value != 9) return 2;
  B derived{{4}, 5};
  C* sliced = new C{derived};
  if (sliced->value != 4) return 3;
  D* nested = new D(1, 2, {3});
  if (nested->c.value != 3) return 4;
  C* designated = new C{.value = 11};
  if (designated->value != 11) return 5;
  int* ints = new int[5]{1, 2, 3};
  if (ints[0] != 1 || ints[2] != 3 || ints[3] != 0 || ints[4] != 0) return 6;
  int* deduced = new int[]{7, 8, 9};
  if (deduced[2] != 9) return 7;
  C* cs = new C[3]{{5}};
  if (cs[0].value != 5 || cs[1].value != 0 || cs[2].value != 0) return 8;
  int n = 4;
  S* ss = new S[n]{1, {2, 3}, S(4)};
  if (ss[0].v != 1 || ss[1].v != 23 || ss[2].v != 4 || ss[3].v != -1) return 9;
  if (live != 4) return 10;
  delete[] ss;
  if (live != 0) return 11;
  Named* named = new Named{"abcdefghijklmnopqrstuvwxyz", 3};
  if (named->name.size() != 26 || named->n != 3) return 12;
  Named* names = new Named[2]{{"x", 1}};
  if (names[0].name != "x" || !names[1].name.empty() || names[1].n != 0) return 13;
  int (*grid)[2] = new int[2][2]{{1, 2}, {3, 4}};
  if (grid[1][0] != 3) return 14;
  int* paren_ints = new int[3](1, 2);
  if (paren_ints[1] != 2 || paren_ints[2] != 0) return 15;
  S (*sgrid)[2] = new S[2][2]{{1, 2}, {3}};
  if (sgrid[0][1].v != 2 || sgrid[1][0].v != 3 || sgrid[1][1].v != -1 || live != 4) return 16;
  delete[] sgrid;
  if (live != 0) return 17;
  int (*elided)[2] = new int[][2]{1, 2, 3};
  if (elided[1][0] != 3 || elided[1][1] != 0) return 18;
  S (*rows)[3] = new S[n][3];
  if (live != 12) return 19;
  delete[] rows;
  if (live != 0) return 20;
  delete[] elided;
  delete copy; delete paren_copy; delete sliced; delete nested; delete designated;
  delete[] ints; delete[] deduced; delete[] cs; delete named; delete[] names;
  delete[] grid; delete[] paren_ints;
  Holder* h = new Holder{Tracked(), 7};
  Holder* hv = new Holder{};
  Holder* hs = new Holder[3]{{Tracked(), 1}};
  if (h->t.self != &h->t || h->k != 7 || hv->t.self != &hv->t || hv->k != 0)
    return 21;
  for (int i = 0; i < 3; ++i)
    if (hs[i].t.self != &hs[i].t || hs[i].k != (i == 0 ? 1 : 0)) return 22;
  if (assignments != 0) return 23;
  delete h; delete hv; delete[] hs;
  return 0;
}
