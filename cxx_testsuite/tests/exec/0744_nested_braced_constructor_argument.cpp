// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Inner {
  int sum;
  Inner(int first, int second) : sum(first + second) {}
};

struct Outer {
  Inner inner;
  int value;
  Outer(Inner inner_value, int value_arg)
      : inner(inner_value), value(value_arg) {}
};

int consume(const Outer& outer) { return outer.inner.sum + outer.value; }

int main() { return consume({{1, 2}, 3}) == 6 ? 0 : 1; }
