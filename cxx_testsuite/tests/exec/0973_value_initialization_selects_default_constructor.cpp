// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <type_traits>

struct NeedsArg {
  NeedsArg(int v) : v(v) {}
  int v;
};

struct Holder {
  NeedsArg value;
  int count;
};

struct Defaulted {
  Defaulted() = default;
  Defaulted(int x) : x(x) {}
  int x;
};

class Private {
  int x = 4;

 public:
  int get() const { return x; }
};

struct Templated {
  template <class U = int>
  Templated(U u = 5) : v(u) {}
  int v;
};

struct DefaultArg {
  DefaultArg(int a = 7) : a(a) {}
  int a;
};

struct WithDefaultArg {
  int i;
  DefaultArg d;
};

struct Poly {
  virtual int f() const { return k; }
  int k;
};

struct VirtualBase {
  int b;
};

struct Derived : virtual VirtualBase {
  int d;
};

struct ConstWithInit {
  const int x = 3;
};

struct MemberWithInit {
  NeedsArg value = NeedsArg(9);
};

struct AnonymousUnion {
  union {
    NeedsArg value;
    int i;
  };
  AnonymousUnion() : i(14) {}
};

union Variant {
  NeedsArg value;
  int i;
  Variant() : i(15) {}
};

struct ExplicitlyInitialized {
  NeedsArg value;
  ExplicitlyInitialized() : value(16) {}
};

struct InitializedByDefault {
  NeedsArg value = NeedsArg(17);
  InitializedByDefault() {}
};

static_assert(!std::is_default_constructible_v<NeedsArg>);
static_assert(!std::is_default_constructible_v<Holder>);
static_assert(std::is_default_constructible_v<Defaulted>);
static_assert(std::is_default_constructible_v<Private>);
static_assert(std::is_default_constructible_v<Templated>);
static_assert(std::is_default_constructible_v<WithDefaultArg>);
static_assert(std::is_default_constructible_v<MemberWithInit>);

int main() {
  Holder h{NeedsArg(6)};
  if (h.value.v != 6 || h.count != 0) return 1;
  Defaulted d{};
  if (d.x != 0) return 2;
  Defaulted e = Defaulted();
  if (e.x != 0) return 3;
  Private p{};
  if (p.get() != 4) return 4;
  Templated t{};
  if (t.v != 5) return 5;
  Templated u = Templated();
  if (u.v != 5) return 6;
  WithDefaultArg w{1};
  if (w.i != 1 || w.d.a != 7) return 7;
  WithDefaultArg x{};
  if (x.i != 0 || x.d.a != 7) return 8;
  Poly poly{};
  if (poly.f() != 0) return 9;
  Derived derived{};
  if (derived.b != 0 || derived.d != 0) return 10;
  ConstWithInit c;
  if (c.x != 3) return 11;
  MemberWithInit m{};
  if (m.value.v != 9) return 12;
  Defaulted* heap = new Defaulted();
  int heap_x = heap->x;
  delete heap;
  if (heap_x != 0) return 13;
  AnonymousUnion anonymous;
  if (anonymous.i != 14) return 14;
  Variant variant;
  if (variant.i != 15) return 15;
  ExplicitlyInitialized explicitly;
  if (explicitly.value.v != 16) return 16;
  InitializedByDefault by_default;
  if (by_default.value.v != 17) return 17;
  return 0;
}
