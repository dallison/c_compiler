// RUN: -std=c++20
struct S {
  int a : 3;
  int : 0;
  int b : 3;
  unsigned : 0, c : 1;
};

int use(S s) { return s.a + s.b + s.c; }
