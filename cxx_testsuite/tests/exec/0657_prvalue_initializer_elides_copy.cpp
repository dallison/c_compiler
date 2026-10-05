// RUN: -std=c++20
// EXPECT_EXIT: 0

// A prvalue of the same class initializes the object directly, with no copy
// ([dcl.init.general]/16.6.1), for variables, members, default member
// initializers and new-expressions alike.  A handler parameter taken by value
// is copy-constructed from the exception object and destroyed with the
// handler.
int copies = 0, live = 0;

struct K {
  int v;
  K() : v(1) { ++live; }
  K(const K& o) : v(o.v) { ++copies; ++live; }
  ~K() { --live; }
};

struct T {
  int v;
  T() : v(2) {}
  T(const T& o) : v(o.v) { ++copies; }
};

K make() { return K(); }
T make_trivial_dtor() { return T(); }

struct Member { K m; Member() : m(make()) {} };
struct Braced { K m; Braced() : m{make()} {} };
struct Default { K m = make(); };
struct TrivialMember { T m; TrivialMember() : m(make_trivial_dtor()) {} };

struct E {
  int value;
  explicit E(int v) : value(v) { ++live; }
  E(const E& o) : value(o.value + 100) { ++copies; ++live; }
  ~E() { --live; }
};

int main() {
  { K k(make()); if (copies != 0 || k.v != 1) return 1; }
  { K k{make()}; if (copies != 0) return 2; }
  { K k = make(); if (copies != 0) return 3; }
  { Member m; if (copies != 0 || m.m.v != 1) return 4; }
  { Braced b; if (copies != 0) return 5; }
  { Default d; if (copies != 0) return 6; }
  { TrivialMember t; if (copies != 0 || t.m.v != 2) return 7; }
  { K* p = new K(make()); if (copies != 0 || p->v != 1) return 8; delete p; }
  if (live != 0) return 9;

  try {
    throw E(11);
  } catch (E caught) {
    if (caught.value != 111 || copies != 1 || live != 2) return 10;
  }
  if (live != 0) return 11;
  copies = 0;
  try {
    throw E(11);
  } catch (const E caught) {
    if (caught.value != 111 || copies != 1) return 12;
  }
  if (live != 0) return 13;
  return 0;
}
