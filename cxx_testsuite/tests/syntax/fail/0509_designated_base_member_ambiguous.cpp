// RUN: -std=c++29
// EXPECT: designated member lookup is ambiguous

struct A {
  int value;
};

struct B {
  int value;
};

struct C : A, B {};

C object{.value = 1};
