// RUN: -std=c++20
// Bit-fields in constant evaluation: each field keeps its own value although
// the fields share a storage unit, and stores truncate (and sign-extend signed
// fields) to the field width.
struct B { int a : 3; int b : 5; unsigned c : 4; };

constexpr B Make(int x) {
  B r{};
  r.a = x;
  r.b = x * 2;
  r.c = x;
  return r;
}
constexpr int Read(int x) {
  B r{};
  r.a = x;
  return r.a;
}
constexpr int ReadB(int x) {
  B r{0, x, 0};
  return r.b;
}
constexpr int Compound(int x) {
  B r{};
  r.a = 3;
  r.a += x;
  return r.a;
}
constexpr int Inc() {
  B r{};
  r.a = 3;
  r.a++;
  return r.a;
}
constexpr int PostDec() {
  B r{};
  r.a = -4;
  int old = r.a--;
  return old * 10 + r.a;
}
constexpr unsigned UnsignedInc() {
  B r{};
  r.c = 15;
  ++r.c;
  return r.c;
}
constexpr int AssignValue() {
  B r{};
  return r.a = 5;
}
constexpr int Local() {
  B b{7, 31, 17};
  return b.a * 100 + b.b * 10 + (int)b.c;
}

struct D { int a : 3 = 7; unsigned b : 2 = 5; };
struct E {
  char c;
  int a : 3;
  char d;
  constexpr E() : c(1), a(6), d(2) {}
};
struct M {
  int x : 4;
  int y : 4;
  constexpr M(int v) : x(v), y(v + 1) {}
};

static_assert(Make(-2).a == -2 && Make(-2).b == -4 && Make(-2).c == 14);
static_assert(Read(3) == 3 && Read(4) == -4);
static_assert(ReadB(-16) == -16 && ReadB(17) == -15);
static_assert(Compound(5) == 0);
static_assert(Inc() == -4);
static_assert(PostDec() == -37);
static_assert(UnsignedInc() == 0);
static_assert(AssignValue() == -3);
static_assert(Local() == -100 - 10 + 1);
static_assert(B{-1, 2, 3}.a == -1 && B{-1, 2, 3}.b == 2 && B{-1, 2, 3}.c == 3);
static_assert(B{7, 31, 17}.a == -1 && B{7, 31, 17}.b == -1 &&
              B{7, 31, 17}.c == 1);
static_assert(D{}.a == -1 && D{}.b == 1);
static_assert(E().c == 1 && E().a == -2 && E().d == 2);
static_assert(M(7).x == 7 && M(7).y == -8);

constexpr B global{-1, 2, 3};
static_assert(global.a == -1 && global.b == 2 && global.c == 3);
