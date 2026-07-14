// RUN: -std=c++20
// EXPECT: through virtual base is not allowed

struct Base {
  int value;
};

struct Derived : virtual Base {
};

int main(void) {
  int Base::*base_member = &Base::value;
  int Derived::*derived_member = base_member;
  (void)derived_member;
  return 0;
}
