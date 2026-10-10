// The active member of a union survives designated initialization in a
// constructor, element-wise array initialization, and copies.
union U { int i; float f; };
struct P { int x; int y; };
union V { P p; double d; };
struct W { int tag; V v; };

struct H { U u; constexpr H() : u{.f = 1.5f} {} };
constexpr H h;
static_assert(h.u.f == 1.5f, "");

constexpr float local() { U g{.f = 2.5f}; return g.f; }
static_assert(local() == 2.5f, "");

constexpr W w{.tag = 1, .v{.d = 4.0}};
static_assert(w.v.d == 4.0, "");
constexpr W w2{.tag = 2, .v = {.p = {3, 4}}};
static_assert(w2.v.p.y == 4, "");

struct A { U arr[2]; constexpr A() : arr{{.f = 1.0f}, {.i = 9}} {} };
constexpr A a;
static_assert(a.arr[0].f == 1.0f && a.arr[1].i == 9, "");

struct Q { V v; constexpr Q() : v{.d = 0.5} {} };
constexpr Q q;
static_assert(q.v.d == 0.5, "");

constexpr U copied() { U x{.i = 3}; U y = x; return y; }
constexpr U c = copied();
static_assert(c.i == 3, "");

constexpr U assigned() { U x{.i = 4}; U y{.f = 1}; y = x; return y; }
constexpr U c2 = assigned();
static_assert(c2.i == 4, "");

constexpr int read_copy() { U x{.i = 5}; U y = x; return y.i; }
static_assert(read_copy() == 5, "");

int main() {
  if (h.u.f != 1.5f) return 1;
  if (a.arr[0].f != 1.0f || a.arr[1].i != 9) return 2;
  if (w.v.d != 4.0 || w2.v.p.y != 4) return 3;
  if (c.i != 3 || c2.i != 4) return 4;
  A runtime;
  if (runtime.arr[0].f != 1.0f || runtime.arr[1].i != 9) return 5;
  return 0;
}
