// RUN: -std=c++17
// EXPECT_EXIT: 0

// Default-initializing an array of class type runs each element's default
// constructor -- for an aggregate, the implicit one that applies the default
// member initializers -- at namespace scope, block scope, for block-scope
// statics, and for an array member of a class whose constructor is implicit,
// user-provided, or instantiated from a template.

struct A {
  int x = 5;
  int y;
};

struct V {
  int x;
  V() : x(7) {}
};

struct B {
  A a[2];
  int z = 9;
};

struct C {
  V v[2];
  A a[2];
  C() {}
};

template <class T>
struct G {
  T t[2];
};

A global[3];
B global_nested[2];

int Local() {
  A local[3];
  B nested[2];
  return local[2].x + nested[1].a[1].x + nested[1].z;
}

int Static() {
  static A statics[2];
  return statics[1].x;
}

int main() {
  if (global[2].x != 5 || global_nested[1].a[1].x != 5 ||
      global_nested[1].z != 9) {
    return 1;
  }
  if (Local() != 19) {
    return 2;
  }
  if (Static() != 5) {
    return 3;
  }
  C c;
  if (c.v[1].x != 7 || c.a[1].x != 5) {
    return 4;
  }
  G<A> ga;
  G<V> gv;
  if (ga.t[1].x != 5 || gv.t[1].x != 7) {
    return 5;
  }
  return 0;
}
