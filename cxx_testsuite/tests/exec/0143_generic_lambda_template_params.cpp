// RUN: -std=c++20
// EXPECT_EXIT: 0
// C++20 generic lambdas with an explicit template-parameter-list.

int main(void) {
  auto add = []<class T>(T a, T b) { return a + b; };
  auto first = []<class T, class U>(T a, U) -> T { return a; };
  auto sz = []<class T>() { return (int)sizeof(T); };

  // Mixed explicit template parameter and abbreviated auto parameter.
  auto mix = []<class T>(T a, auto b) { return a + b; };

  // Capture combined with an explicit template-parameter-list.
  int base = 100;
  auto addbase = [base]<class T>(T x) { return base + (int)x; };

  int r = 0;
  r += add(3, 4);                 // 7
  r += (int)(add(1.5, 2.5) + 0);  // 4
  r += first(10, 2.0);            // 10
  r += sz.operator()<int>();      // sizeof(int) == 4
  r += mix(3, 4);                 // 7
  r += (int)mix(1, 2.5);          // 3
  r += addbase(5);                // 105
  r += addbase(2.0);              // 102
  // 7 + 4 + 10 + 4 + 7 + 3 + 105 + 102 = 242
  return r == 242 ? 0 : 1;
}
