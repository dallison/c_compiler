// RUN: -std=c++20
// A class prvalue initializes its result object directly, so a pointer the
// object holds into itself designates the object it initializes.  A
// constructor that is not a trivial copy runs its own body.

struct SelfRef {
  int buf[2];
  int* p;
  constexpr SelfRef(int v) : buf{v, 0}, p(buf) {}
  constexpr SelfRef(const SelfRef& o) : buf{*o.p, 0}, p(buf) {}
  constexpr SelfRef(const SelfRef& o, int d) : buf{*o.p + d, 0}, p(buf) {}
  constexpr ~SelfRef() {}
  constexpr SelfRef make(int d) const { return SelfRef(*this, d); }
  constexpr int value() const { return *p; }
  constexpr int raw() const { return buf[0]; }
};

constexpr bool initializes_variable() {
  SelfRef a(3);
  SelfRef b = a.make(4);
  return b.p == b.buf && b.value() == 7;
}
static_assert(initializes_variable());

constexpr bool member_call_on_prvalue() {
  SelfRef a(3);
  return a.make(4).raw() == 7 && a.make(5).value() == 8;
}
static_assert(member_call_on_prvalue());

struct Tracked {
  int buf[1];
  int* p;
  constexpr Tracked(int v) : buf{v}, p(buf) {}
  constexpr ~Tracked() {}
};

struct Holder {
  Tracked t{5};
  constexpr Tracked get() const { return t; }
};

// The implicit copy copies the pointer, which still designates the source.
constexpr bool copy_keeps_source_pointer() {
  Holder h;
  Tracked c = h.get();
  return c.p == h.t.buf;
}
static_assert(copy_keeps_source_pointer());

struct Counted {
  int v;
  constexpr Counted(int x) : v(x) {}
  constexpr Counted(const Counted& o) : v(o.v + 1) {}
  constexpr Counted(int d, const Counted& o) : v(o.v + d) {}
};

constexpr int user_copy() {
  Counted a(3);
  return Counted(a).v;
}
static_assert(user_copy() == 4);

constexpr int trailing_class_argument() {
  Counted a(3);
  return Counted(10, a).v;
}
static_assert(trailing_class_argument() == 13);
