// RUN: -std=c++20
// EXPECT: 0593_constexpr_constructor_leaves_member_indeterminate.cpp:16: constexpr variable initializer is not a constant expression
// EXPECT: 0593_constexpr_constructor_leaves_member_indeterminate.cpp:19: constexpr variable initializer is not a constant expression
// EXPECT: 0593_constexpr_constructor_leaves_member_indeterminate.cpp:23: constexpr variable initializer is not a constant expression
// EXPECT: 0593_constexpr_constructor_leaves_member_indeterminate.cpp:26: constexpr variable initializer is not a constant expression
// EXPECT: 0593_constexpr_constructor_leaves_member_indeterminate.cpp:29: constexpr variable initializer is not a constant expression
// EXPECT: 0593_constexpr_constructor_leaves_member_indeterminate.cpp:33: constexpr variable initializer is not a constant expression

// A user-provided constructor leaves every member it does not initialize
// indeterminate, whether the object is static, an array element, a member, a
// base, or a local read inside a constant evaluation ([expr.const],
// [dcl.init.general]).

struct In { int v; };
struct D { In in; int k; constexpr D() : k(3) {} };
constexpr D d;

struct E { int a[3]; constexpr E() {} };
constexpr E e;

struct Base { int p; constexpr Base() {} };
struct G : Base { int q; constexpr G() : q(5) {} };
constexpr G g;

struct Q { int a; int b; constexpr Q(int v) : a(v) {} };
constexpr Q qs[2] = {Q(1), Q(2)};

constexpr int ReadLocal() { Q q(1); return q.b; }
constexpr int r = ReadLocal();

struct E2 { int a[2]; constexpr E2() {} };
constexpr int ReadLocalElement() { E2 e2; return e2.a[1]; }
constexpr int r2 = ReadLocalElement();

int main() { return 0; }
