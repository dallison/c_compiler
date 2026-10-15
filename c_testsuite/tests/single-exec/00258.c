// Conditional operator arms of one structure or union type (C11 6.5.15p3):
// lvalue, call, and mixed arms, as a value, an operand, and an argument.
#include <stdio.h>

struct S { unsigned long long lo, hi; };
struct Big { int v[9]; };
union U { int i; float f; };
static struct S Make(unsigned long long lo) { struct S s = {lo, lo + 1}; return s; }
static struct S Neg(struct S v) { v.lo = -v.lo; return v; }
static struct Big MakeBig(int base) { struct Big b; for (int i = 0; i < 9; i++) b.v[i] = base + i; return b; }
struct S Pick(int c, struct S a, struct S b) { return c ? a : b; }
struct S Mixed(int c, struct S a) { return c ? Neg(a) : a; }
struct S Calls(int c) { return c ? Make(10) : Make(20); }
struct Big PickBig(int c) { struct Big local = MakeBig(100); return c ? MakeBig(1) : local; }
unsigned long long Sum(struct S s) { return s.lo + s.hi; }
int main(void) {
  struct S a = {1, 2}, b = {3, 4};
  const struct S k = {5, 6};
  if (Pick(0, a, b).lo != 3 || Pick(1, a, b).hi != 2) return 1;
  if (Mixed(1, a).lo != (unsigned long long)-1 || Mixed(0, a).lo != 1) return 2;
  if (Calls(1).lo != 10 || Calls(0).hi != 21) return 3;
  struct Big big = PickBig(1), big2 = PickBig(0);
  if (big.v[8] != 9 || big2.v[0] != 100 || big2.v[8] != 108) return 4;
  int c = (int)Sum(a) - 3;
  if ((c ? a : b).hi != 4) return 5;
  struct S s;
  s = c ? a : k;
  if (s.lo != 5) return 6;
  if (Sum(c == 0 ? k : b) != 11) return 7;
  union U u1 = {7}, u2 = {9};
  union U u = c ? u1 : u2;
  if (u.i != 9) return 8;
  puts("ok");
  return 0;
}
