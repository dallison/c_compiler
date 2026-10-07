// RUN: -std=c++20
// EXPECT: 0597_constexpr_new_class_misuse_2.cpp:19: constexpr variable initializer is not a constant expression
// EXPECT: 0597_constexpr_new_class_misuse_2.cpp:27: constexpr variable initializer is not a constant expression
// EXPECT: 0597_constexpr_new_class_misuse_2.cpp:30: constexpr variable initializer is not a constant expression

// Class objects from new-expressions in constant evaluation: deleting from the
// middle of an array, reading a member that default-initialization left
// indeterminate, and keeping a pointer to a deleted object are ill-formed
// ([expr.const]).

struct J { int a; constexpr J(int v) : a(v) {} };
struct S { int a; };

constexpr int DeleteInterior() {
  J* p = new J[2]{J(1), J(2)};
  delete[] (p + 1);
  return 1;
}
constexpr int r4 = DeleteInterior();

constexpr int ReadIndeterminate() {
  S* p = new S;
  int v = p->a;
  delete p;
  return v;
}
constexpr int r5 = ReadIndeterminate();

constexpr J* Dangling() { J* p = new J(1); delete p; return p; }
constexpr J* r6 = Dangling();

int main() { return 0; }
