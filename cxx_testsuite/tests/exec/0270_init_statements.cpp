// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// C++17 init-statements: if / switch may carry an init-statement before the
// controlling condition, "if (init; cond)".  The init-statement (an
// expression-statement or a simple-declaration) and any names it declares are
// in scope for the condition and both controlled statements.

int g_ctors = 0;
int g_dtors = 0;

struct Guard {
  int v;
  explicit Guard(int x) : v(x) { g_ctors++; }
  ~Guard() { g_dtors++; }
};

int run() {
  // if with an init-declaration; the name is visible in the condition and body.
  int a = 0;
  if (int n = 5; n > 3) {
    a = n;
  } else {
    a = -1;
  }
  if (a != 5) return 1;

  // The init-declared name is also visible in the else branch.
  if (int n = 1; n > 3) {
    a = 100;
  } else {
    a = n + 6;
  }
  if (a != 7) return 2;

  // The init-statement may be an expression-statement.
  int side = 0;
  if (side = 9; side == 9) {
    a = side;
  }
  if (a != 9) return 3;

  // switch with an init-declaration.
  int s = 0;
  switch (int k = 2; k + 1) {
    case 3: s = 30 + k; break;
    default: s = -1; break;
  }
  if (s != 32) return 4;

  // init-statement combined with a condition-declaration: both are in scope.
  int combo = 0;
  if (int base = 10; int off = base / 5) {
    combo = base + off;  // base = 10, off = 2
  } else {
    combo = -1;
  }
  if (combo != 12) return 5;

  // An init-declared object with a destructor is destroyed at the end of the
  // if statement (both the init-statement and condition scopes close).
  a = 0;
  {
    if (Guard gg(7); gg.v == 7) {
      a = gg.v;
    }
  }
  if (a != 7) return 6;
  if (g_ctors != 1 || g_dtors != 1) return 7;

  // Regression: a plain condition with no init-statement is unaffected.
  int z = 4;
  if (z < 10) a = 1; else a = 0;
  if (a != 1) return 8;

  return 0;
}

int main() { return run(); }
