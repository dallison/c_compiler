// RUN: -std=c++20
// EXPECT_EXIT: 0
// C++20 abbreviated function templates, including cv-qualified / reference
// forms of the `auto` placeholder.

int plain(auto x) { return (int)x; }
int by_cval(const auto x) { return (int)x; }
int by_cref(const auto& x) { return (int)x; }
int by_ptr(auto* p) { return (int)*p; }

// Sum via forwarding reference; deduced independently at each call.
auto sum(auto a, auto b) { return a + b; }

int main(void) {
  int v = 7;

  // Deduce at several types.
  int a = plain(3) + by_cval(4) + by_cref(5) + by_ptr(&v);  // 3+4+5+7 = 19

  int i = sum(2, 3);              // int: 5
  double d = sum(1.5, 2.25);      // double: 3.75

  // Abbreviated parameter in a generic lambda, cv-qualified.
  auto twice = [](const auto& x) { return x + x; };
  int lt = twice(6);             // 12
  double ld = twice(1.5);        // 3.0

  int total = a + i + (int)(d * 100) + lt + (int)(ld * 10);
  // 19 + 5 + 375 + 12 + 30 = 441
  return total == 441 ? 0 : 1;
}
