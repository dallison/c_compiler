// RUN: -std=c++17
// EXPECT_EXIT: 0

// Namespace-scope arrays of class type copy-initialized element by element
// through a converting constructor ([dcl.init.aggr]): the constructor runs
// during dynamic initialization, so the initializer need not be constant.

int constructed;

struct V {
  int x;
  V(int v) : x(v) { constructed++; }
};

V ws[] = {1, 2, 3};

struct W {
  int x;
  W(int v = 5) : x(v) {}
};
W fixed[4] = {10, 20};
W defaults[3] = {1};

int main() {
  if (sizeof(ws) != 3 * sizeof(V)) {
    return 1;
  }
  if (ws[0].x != 1 || ws[2].x != 3) {
    return 2;
  }
  if (fixed[1].x != 20 || fixed[3].x != 5) {
    return 3;
  }
  if (defaults[0].x != 1 || defaults[2].x != 5) {
    return 4;
  }
  return constructed == 3 ? 0 : 5;
}
