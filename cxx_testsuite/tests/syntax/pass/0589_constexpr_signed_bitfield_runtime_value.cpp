// RUN: -std=c++17 -fconstexpr-eval=audit
// A signed bit-field set from a function parameter reads back sign-extended.

struct Bits {
  int a : 5;
  unsigned b : 3;
};

constexpr int Signed(int i) {
  Bits s{i - 10, 5};
  return s.a;
}

constexpr int Unsigned(int i) {
  Bits s{i - 10, 5};
  return s.b;
}

constexpr int Selected(int i) {
  Bits s{i - 10, 5};
  int a = (i > 100 ? 0 : s.a);
  int b = (i > 100 ? 1 : s.b);
  return a * 100 + b;
}

static_assert(Signed(7) == -3);
static_assert(Signed(25) == 15);
static_assert(Unsigned(7) == 5);
static_assert(Selected(7) == -295);

int main() { return 0; }
