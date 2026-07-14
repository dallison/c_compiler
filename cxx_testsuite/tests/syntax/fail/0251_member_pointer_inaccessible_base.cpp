// RUN: -std=c++20
// EXPECT: incompatible with member class

struct Base {
  int value;
};

struct Derived : private Base {
};

int main(void) {
  int Base::*member = &Base::value;
  Derived object;
  return object.*member;
}
