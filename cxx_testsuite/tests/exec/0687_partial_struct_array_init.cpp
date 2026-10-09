// RUN: -std=c++20
// EXPECT_EXIT: 0
// Arrays of structs initialized by fewer brace elements than they hold, in
// constant and static initialization ([dcl.init.aggr]): remaining members and
// elements take their default member initializers or are value-initialized.

struct D { int a; int b; int c; };
struct I { int a; int b = 5; };

constexpr D ds[2] = {{1, 2}};
constexpr D dt[] = {{1, 2}, {3}};
constexpr I is[] = {{8}};
constexpr I it[3] = {{8}, {9, 1}};
D gd[] = {{1}};
I gi[2] = {{4}};

static_assert(ds[0].a == 1 && ds[0].b == 2 && ds[0].c == 0);
static_assert(ds[1].a == 0 && ds[1].b == 0 && ds[1].c == 0);
static_assert(dt[1].a == 3 && dt[1].b == 0 && sizeof(dt) == 2 * sizeof(D));
static_assert(is[0].a == 8 && is[0].b == 5 && sizeof(is) == sizeof(I));
static_assert(it[1].b == 1 && it[2].a == 0 && it[2].b == 5);

constexpr int Local() {
  D l[2] = {{1, 2}};
  I m[] = {{8}, {9, 1}};
  I n[3] = {{7}};
  return l[0].b + l[1].c + m[0].b + m[1].b + n[2].b;
}
static_assert(Local() == 2 + 0 + 5 + 1 + 5);

template <class T, int N> int Sum(const T (&v)[N]) {
  int s = 0;
  for (int i = 0; i < N; i++) s += v[i].a * 10 + v[i].b;
  return s;
}

int main() {
  // Read through a function so the stored static data is checked, not folds.
  if (Sum(ds) != 12) return 1;
  if (Sum(dt) != 12 + 30) return 2;
  if (Sum(is) != 85) return 3;
  if (Sum(it) != 85 + 91 + 5) return 4;
  if (gd[0].a != 1 || gd[0].b != 0 || gd[0].c != 0) return 5;
  if (Sum(gi) != 45 + 5) return 6;
  if (Local() != 13) return 7;
  return 0;
}
