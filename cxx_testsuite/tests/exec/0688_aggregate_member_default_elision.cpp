// RUN: -std=c++20
// EXPECT_EXIT: 0
// Brace elision into a member struct that has a default member initializer,
// and omitted trailing array members ([dcl.init.aggr]): an elided brace takes
// explicit values first, and only omitted members use their defaults.

struct Inner { int p; int q = 7; };
struct Outer { Inner in; int z; int arr[2]; };

constexpr Outer o = {1, 2, 3, 4};
constexpr Outer o2 = {{1}, 2};
constexpr Outer o3 = {{1, 9}, 2, {5}};
Outer g = {1, 2, 3, 4};
Outer g2 = {{1}, 2};

static_assert(o.in.p == 1 && o.in.q == 2 && o.z == 3);
static_assert(o.arr[0] == 4 && o.arr[1] == 0);
static_assert(o2.in.p == 1 && o2.in.q == 7 && o2.z == 2);
static_assert(o2.arr[0] == 0 && o2.arr[1] == 0);
static_assert(o3.in.q == 9 && o3.arr[0] == 5 && o3.arr[1] == 0);

constexpr int Local() {
  Outer l = {1, 2, 3, 4};
  Outer m = {{1}, 2};
  return l.in.q * 100 + m.in.q * 10 + m.arr[1];
}
static_assert(Local() == 270);

int Check(const Outer& v, int p, int q, int z, int a0, int a1) {
  return v.in.p == p && v.in.q == q && v.z == z && v.arr[0] == a0 &&
         v.arr[1] == a1;
}

int main() {
  if (!Check(o, 1, 2, 3, 4, 0)) return 1;
  if (!Check(o2, 1, 7, 2, 0, 0)) return 2;
  if (!Check(o3, 1, 9, 2, 5, 0)) return 3;
  if (!Check(g, 1, 2, 3, 4, 0)) return 4;
  if (!Check(g2, 1, 7, 2, 0, 0)) return 5;
  if (Local() != 270) return 6;
  return 0;
}
