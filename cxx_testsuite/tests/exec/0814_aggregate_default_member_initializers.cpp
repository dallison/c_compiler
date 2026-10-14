// RUN: -std=c++17
// EXPECT_EXIT: 0

// A braced default member initializer for an aggregate member is aggregate
// initialization, including for unions and unnamed class types, and a name in
// it that denotes an earlier class-type data member refers to that member of
// the object being initialized.  Protobuf's MessageDifferencer keeps
// `union { Default* d; Base* b; } field_ = {&default_};`.

struct Base {
  virtual ~Base() {}
  virtual int f() const { return 1; }
};
struct Default : Base {
  int f() const override { return 2; }
};

struct Pair {
  int a;
  int b;
};
struct Inner {
  int v;
};
struct Outer {
  Inner i;
  int b;
};

class Differencer {
 public:
  Differencer() {}
  int Use() const {
    return kind_ == kDefault ? comparator_.default_impl->f()
                             : comparator_.base->f();
  }
  void Set(Base* b) {
    kind_ = kBase;
    comparator_.base = b;
  }

 private:
  Default default_;
  union {
    Default* default_impl;
    Base* base;
  } comparator_ = {&default_};
  enum { kDefault, kBase } kind_ = kDefault;
};

struct Members {
  Pair full = {7, 8};
  Pair partial{5};
  Outer nested = {{3}, 4};
  Inner inner;
  Inner* self = &inner;
  struct {
    Inner* p;
    long* q;
  } unnamed = {&inner};
};

int main() {
  Differencer d;
  if (d.Use() != 2) return 1;
  Base b;
  d.Set(&b);
  if (d.Use() != 1) return 2;

  Members m;
  if (m.full.a != 7 || m.full.b != 8) return 3;
  if (m.partial.a != 5 || m.partial.b != 0) return 4;
  if (m.nested.i.v != 3 || m.nested.b != 4) return 5;
  if (m.self != &m.inner || m.unnamed.p != &m.inner || m.unnamed.q) return 6;
  return 0;
}
