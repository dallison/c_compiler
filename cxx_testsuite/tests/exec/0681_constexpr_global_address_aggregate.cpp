// RUN: -std=c++20
// EXPECT_EXIT: 0
// Constexpr aggregates holding addresses of non-constant globals, of their
// elements and members, and of elements of constant arrays are emitted with
// those addresses.

int g;
int h[3];
struct S {
  int a;
  int b;
} s;
struct Outer {
  int x;
  S in;
  int arr[2];
} outer;
constexpr int table[3] = {10, 20, 30};

struct Refs {
  int* p;
  int* q;
  int* r;
};
constexpr Refs refs = {&s.b, h + 2, &outer.in.b};

constexpr int* ps[] = {&g, &h[1], &outer.arr[1], h + 3};
constexpr const int* cs[] = {&table[1], table + 2};

constexpr Refs MakeRefs() { return {&g, &outer.arr[0], &outer.x}; }
constexpr Refs made = MakeRefs();

constexpr bool Distinct(const int* p, const int* q) { return p != q; }
static_assert(Distinct(&g, &h[0]));
static_assert(refs.p == &s.b && refs.q == &h[2] && refs.r == &outer.in.b);
static_assert(ps[2] == &outer.arr[1] && ps[3] == h + 3);
static_assert(cs[0] == &table[1] && *cs[1] == 30);

int main() {
  if (refs.p != &s.b || refs.q != &h[2] || refs.r != &outer.in.b) {
    return 1;
  }
  if (ps[0] != &g || ps[1] != &h[1] || ps[2] != &outer.arr[1] ||
      ps[3] != h + 3) {
    return 2;
  }
  if (cs[0] != &table[1] || cs[1] != &table[2] || *cs[0] != 20) {
    return 3;
  }
  if (made.p != &g || made.q != &outer.arr[0] || made.r != &outer.x) {
    return 4;
  }
  *refs.p = 7;
  *made.q = 9;
  if (s.b != 7 || outer.arr[0] != 9) {
    return 5;
  }
  return 0;
}
