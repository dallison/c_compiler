// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Big {
  long a;
  long b;
  long c;
  long d;
  long e;
  long f;

  Big(long a0, long b0, long c0, long d0, long e0, long f0)
      : a(a0), b(b0), c(c0), d(d0), e(e0), f(f0) {}

  Big(const Big& other)
      : a(other.a),
        b(other.b),
        c(other.c),
        d(other.d),
        e(other.e),
        f(other.f) {}
};

struct Wrapper {
  Big value;

  [[gnu::noinline]] explicit Wrapper(Big input) : value(input) {}
};

int main() {
  Big input(1, 2, 3, 4, 5, 6);
  Wrapper wrapped(input);
  return wrapped.value.a == 1 && wrapped.value.b == 2 &&
                 wrapped.value.c == 3 && wrapped.value.d == 4 &&
                 wrapped.value.e == 5 && wrapped.value.f == 6
             ? 0
             : 1;
}
