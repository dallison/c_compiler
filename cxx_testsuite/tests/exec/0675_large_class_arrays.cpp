// RUN: -std=c++17
// EXPECT_EXIT: 0

// Arrays of class type too long to construct with one call per element are
// constructed and destroyed by a loop: at namespace scope, as thread_locals,
// as block-scope statics and locals, and as array members.  Elements are
// destroyed last to first.  Also checks the bound of a dynamically
// initialized array of unknown bound whose initializers are string literals.

#include <stdlib.h>
#include <string>

int constructed;
int next_destroyed = -1;

struct Ordered {
  int id;
  Ordered() : id(constructed++) {}
  ~Ordered() {
    if (next_destroyed >= 0 && id != next_destroyed) {
      _Exit(20);
    }
    next_destroyed = id - 1;
  }
};

struct Aggregate {
  int value = 3;
  int other;
};

struct V {
  int x;
  V() : x(7) {}
};

struct Members {
  V v[1000];
  Aggregate a[1000];
};

// Runs last at exit: every Ordered element was destroyed in reverse order.
struct Check {
  ~Check() {
    if (next_destroyed != -1) {
      _Exit(21);
    }
  }
} check;

Ordered ordered[4096];
Aggregate aggregates[1 << 16];
V grid[300][3];
thread_local V per_thread[1000];

const char* Name() { return "n"; }
std::string strings[][2] = {"a", "b", "c"};
static_assert(sizeof(strings) == 4 * sizeof(std::string), "");
const char* names[][2] = {"a", "b", Name()};
static_assert(sizeof(names) == 4 * sizeof(const char*), "");

int Local() {
  Aggregate local[50000];
  static V statics[30000];
  Members* members = new Members;
  int sum = local[49999].value + statics[29999].x + members->v[999].x +
            members->a[999].value;
  delete members;
  return sum;
}

int main() {
  if (constructed != 4096 || ordered[4095].id != 4095) {
    return 1;
  }
  if (aggregates[65535].value != 3 || grid[299][2].x != 7) {
    return 2;
  }
  if (per_thread[999].x != 7) {
    return 3;
  }
  if (Local() != 20) {
    return 4;
  }
  if (strings[1][0] != "c" || names[1][0][0] != 'n') {
    return 5;
  }
  next_destroyed = 4095;
  return 0;
}
