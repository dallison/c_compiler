// RUN: -std=c++20

// Each constructor initializes every member one way or another, so all of
// these objects are constants: value-initialization zeroes before a defaulted
// constructor, and members may be set by mem-initializers, value-initializing
// mem-initializers, default member initializers, a base or delegated
// constructor, or the constructor body.

struct B { int x; int z = 1; B() = default; };
constexpr B b{};
static_assert(b.x == 0 && b.z == 1);

struct C { int x; int y; constexpr C() : x(1) { y = 2; } };
constexpr C c;
static_assert(c.y == 2);

struct In { int v; };
struct D { In in; int k; constexpr D() : in(), k(3) {} };
constexpr D d;
static_assert(d.in.v == 0);

struct E { int a[3]; constexpr E() : a{} {} };
constexpr E e;
static_assert(e.a[2] == 0);

struct F {
  int a[3];
  constexpr F() {
    for (int i = 0; i < 3; i++) a[i] = i;
  }
};
constexpr F f;
static_assert(f.a[2] == 2);

struct Base { int p; constexpr Base(int v) : p(v) {} };
struct G : Base { int q; constexpr G() : Base(4), q(5) {} };
constexpr G g;
static_assert(g.p == 4 && g.q == 5);

struct I { int x; constexpr I(int v) : x(v) {} constexpr I() : I(9) {} };
constexpr I i9;
static_assert(i9.x == 9);

struct J { int a; int b; constexpr J(int v) : a(v) { b = a + 1; } };
constexpr int ReadLocals() {
  J j(1);
  J* p = new J(2);
  int v = j.b + p->b;
  delete p;
  return v;
}
static_assert(ReadLocals() == 5);

struct K { int a = 1; int b; constexpr K() { b = 2; } };
constexpr int ReadElement() {
  K ks[3];
  return ks[2].a + ks[2].b;
}
static_assert(ReadElement() == 3);

int main() { return 0; }
