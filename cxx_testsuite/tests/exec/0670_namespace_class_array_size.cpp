// RUN: -std=c++17
// EXPECT_EXIT: 0

// A namespace-scope array of class type whose bound comes from its
// initializer list: sizeof must count the elements, as it does for a local.

struct V {
  int x;
  V() : x(7) {}
  V(int v) : x(v) {}
};

V vs[] = {V(), V(3)};
static_assert(sizeof(vs) == 2 * sizeof(V), "bound from the initializer");

int Count() { return sizeof(vs) / sizeof(vs[0]); }

int main() {
  if (Count() != 2) {
    return 1;
  }
  if (vs[0].x != 7 || vs[1].x != 3) {
    return 2;
  }
  V ls[] = {V(), V(3)};
  if (sizeof(ls) != sizeof(vs)) {
    return 3;
  }
  return 0;
}
