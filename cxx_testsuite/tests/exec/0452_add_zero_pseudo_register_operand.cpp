// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0
// `T() + 5` lowers the value-initialized operand to the zero pseudo-register,
// which has no physical register behind it.  When the other operand already
// occupied the destination register the emitter swapped the two, but kept the
// answer to "is the second operand zero" from before the swap, so it printed
// the pseudo-register as %rax and added whatever the last call had returned.

template <class T>
struct Wrap {
  T value;
  Wrap() { value = T(); }
  explicit Wrap(T v) { value = v; }
  T get() { return value; }
};

template <class T>
struct User {
  T total;
  User() { total = Wrap<T>().get() + Wrap<T>(T() + 5).get(); }
};

// Runs first so that a non-zero value is left in the call-return register.
static int prime(int x) {
  return Wrap<int>().get() + Wrap<int>(x).get();
}

int main(void) {
  if (prime(4) != 4) {
    return 1;
  }

  User<int> u;
  if (u.total != 5) {
    return 2;
  }

  User<long> l;
  if (l.total != 5) {
    return 3;
  }

  return 0;
}
