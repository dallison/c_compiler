// RUN: -std=c++20
// EXPECT: No such symbol "X"
// EXPECT: No such symbol "Y"
// EXPECT-NOT: Template argument must name a type
// EXPECT-NOT: Template non-type argument must be an integer constant
// EXPECT-NOT: Template argument deduction failed
// An undeclared template argument is reported once; matching it to the
// template's parameters, or deducing the call, adds no further errors.

template <typename T, typename T1> constexpr int Find() { return 0; }

template <typename T> struct A {
  template <typename U> int Off() const { return Find<U, X>(); }
};

int Direct() { return Find<int, Y>(); }

int main() {
  A<int> a;
  return a.Off<char>() + Direct();
}
