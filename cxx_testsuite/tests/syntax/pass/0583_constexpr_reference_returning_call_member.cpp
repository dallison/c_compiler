// RUN: -std=c++20 -fconstexpr-eval=audit

// A member of the class object a call returns by reference is the referent's
// member: reads see it, writes through it change it, and member calls run on
// it.

struct P {
  int a;
  constexpr P(int x) : a(x) {}
  constexpr int Twice() const { return 2 * a; }
};

struct Q {
  int a;
  int b;
};

constexpr P& Ref(P& p) { return p; }
constexpr const P& ConstRef(const P& p) { return p; }
constexpr Q& RefQ(Q& q) { return q; }

struct Holder {
  P p;
  constexpr Holder() : p(4) {}
  constexpr P& Get() { return p; }
};

constexpr int Read() {
  P p(7);
  return Ref(p).a;
}

constexpr int ReadAggregate() {
  Q q{3, 9};
  return RefQ(q).a * 10 + RefQ(q).b;
}

constexpr int Write() {
  P p(1);
  Ref(p).a = 5;
  return p.a;
}

constexpr int Nested() {
  P p(6);
  return Ref(Ref(p)).a + ConstRef(p).a;
}

constexpr int MemberCall() {
  P p(8);
  return Ref(p).Twice();
}

constexpr int MemberReturningReference() {
  Holder h;
  h.Get().a += 3;
  return h.Get().a;
}

static_assert(Read() == 7);
static_assert(ReadAggregate() == 39);
static_assert(Write() == 5);
static_assert(Nested() == 12);
static_assert(MemberCall() == 16);
static_assert(MemberReturningReference() == 7);
