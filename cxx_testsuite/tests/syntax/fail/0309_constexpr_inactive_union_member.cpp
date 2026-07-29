// RUN: -std=c++20
// EXPECT: static_assert expression is not an integer constant expression

union Value {
  int integer;
  long other;

  constexpr Value(long value) : other(value) {}
};

constexpr int read_inactive_member() {
  Value value(7L);
  return value.integer;
}

static_assert(read_inactive_member() == 7);
