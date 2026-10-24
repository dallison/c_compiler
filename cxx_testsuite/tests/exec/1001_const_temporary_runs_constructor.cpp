// RUN: -std=c++17
// EXPECT_EXIT: 0

// Binding `const T&` to `{}` materializes a temporary. That temporary is
// const, but it is not a constant initializer: its constructor must run.

static int constructed;

struct Inner {
  int v;
  Inner() : v(3) { constructed += 1; }
};

struct Options {
  Inner inner;
};

int observe(const Options& options) {
  return options.inner.v + constructed;
}

int main() {
  if (observe({}) != 4) {
    return 1;
  }
  if (observe({}) != 5) {
    return 2;
  }
  return 0;
}
