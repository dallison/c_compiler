// RUN: -std=c++29
// EXPECT: designated member is inherited through a non-aggregate base class

struct NonAggregate {
  int value;
  NonAggregate(int);
};

struct Derived : NonAggregate {
  int member;
};

Derived object{.value = 1, .member = 2};
