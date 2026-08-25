// RUN: -std=c++11
struct Outer {
  struct Inner {
    int intfield;
  };
};

class A {
  friend class Outer::Inner;
};

int use(Outer::Inner inner) { return inner.intfield; }
