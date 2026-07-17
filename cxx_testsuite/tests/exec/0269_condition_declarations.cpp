// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// C++ condition-declarations: a declaration with an initializer may appear as
// the controlling condition of if / switch / while.  The declared variable is
// in scope for the controlled statement(s) and its value (contextually
// converted to bool) is the condition.

int g_ctors = 0;
int g_dtors = 0;

struct Counted {
  int value;
  explicit Counted(int v) : value(v) { g_ctors++; }
  Counted(const Counted& o) : value(o.value) { g_ctors++; }
  ~Counted() { g_dtors++; }
  explicit operator bool() const { return value != 0; }
};

int g_seq[4] = {3, 2, 0, 9};
int g_idx = 0;
int next_value() { return g_seq[g_idx++]; }

int run() {
  // if: variable visible in the taken branch.
  int a = 0;
  if (int x = 5) {
    a = x;
  } else {
    a = -1;
  }
  if (a != 5) return 1;

  // if: variable visible in the else branch when the condition is false.
  if (int x = 0) {
    a = 100;
  } else {
    a = x + 7;
  }
  if (a != 7) return 2;

  // switch on a declared variable.
  int s = 0;
  switch (int y = 2) {
    case 1: s = 10; break;
    case 2: s = 20 + y; break;
    default: s = -1; break;
  }
  if (s != 22) return 3;

  // while: the variable is re-created and destroyed on every iteration.
  int count = 0;
  while (Counted c = Counted(next_value())) {
    count++;
    if (count > 10) return 4;
  }
  // next_value yields 3, 2, 0: two truthy iterations, then a false one.
  if (count != 2) return 5;
  if (g_ctors != g_dtors || g_ctors == 0) return 6;  // balanced, no leak

  // pointer condition-declaration.
  int arr[3] = {5, 6, 0};
  int i = 0, sum = 0;
  while (int* p = &arr[i]) {
    if (*p == 0) break;
    sum += *p;
    i++;
  }
  if (sum != 11) return 7;

  // continue re-evaluates (re-creates) the condition variable.
  int arr2[4] = {1, 2, 3, 0};
  int j = 0, total = 0;
  while (int v = arr2[j]) {
    j++;
    if (v == 2) continue;
    total += v;
  }
  if (total != 4) return 8;  // 1 + 3

  // Regression: expression conditions that begin with a type-like token are
  // still parsed as expressions, not declarations.
  int z = 4;
  if (bool(z)) a = 1; else a = 0;
  if (a != 1) return 9;
  if ((int)z > 0) a = 2; else a = 3;
  if (a != 2) return 10;

  return 0;
}

int main() { return run(); }
