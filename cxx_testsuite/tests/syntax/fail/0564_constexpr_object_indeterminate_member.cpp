// RUN: -std=c++20
// EXPECT: constexpr variable initializer is not a constant expression

// A constexpr object must be fully initialized by a constant expression
// ([expr.const]): the constructor leaves `b` indeterminate, so `q` is
// ill-formed, as is reading it.

struct Q {
  int a;
  int b;
  constexpr Q() : a(1) {}
};

constexpr Q q;

int main() { return q.a; }
