// RUN: -std=c++20
// EXPECT: incompatible with member class

struct A {
  int value;
};

struct B {
  int value;
};

int main(void) {
  int A::*member = &A::value;
  B object{1};
  return object.*member;
}
