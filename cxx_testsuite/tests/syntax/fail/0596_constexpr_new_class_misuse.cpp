// RUN: -std=c++20
// EXPECT: 0596_constexpr_new_class_misuse.cpp:14: constexpr variable initializer is not a constant expression
// EXPECT: 0596_constexpr_new_class_misuse.cpp:17: constexpr variable initializer is not a constant expression
// EXPECT: 0596_constexpr_new_class_misuse.cpp:20: constexpr variable initializer is not a constant expression

// Class objects from new-expressions in constant evaluation: reading one after
// delete, deleting it twice and never deleting it are ill-formed
// ([expr.const]).  0597 has more cases; together they would exceed the error
// limit.

struct J { int a; constexpr J(int v) : a(v) {} };

constexpr int UseAfterDelete() { J* p = new J(1); delete p; return p->a; }
constexpr int r1 = UseAfterDelete();

constexpr int DoubleDelete() { J* p = new J(1); delete p; delete p; return 1; }
constexpr int r2 = DoubleDelete();

constexpr int Leak() { J* p = new J(1); return p->a; }
constexpr int r3 = Leak();

int main() { return 0; }
