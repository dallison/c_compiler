// RUN: -std=c++20
#include <type_traits>

struct Base {
  int a;
  int b;
  int get(void) const { return a + b; }
};

struct Derived : Base {
  int c;
  int get(void) const { return a + b + c; }
};

static_assert(std::is_member_pointer<int Base::*>::value);
static_assert(std::is_member_pointer<int (Base::*)() const>::value);
static_assert(sizeof(int Base::*) == 8);
static_assert(sizeof(int (Base::*)() const) == 16);

int main(void) {
  Base base{1, 2};
  int Base::*pa = &Base::a;
  int Base::*pb = &Base::b;
  int (Base::*pf)(void) const = &Base::get;

  if (base.*pa != 1) return 1;
  if (base.*pb != 2) return 2;
  if ((base.*pf)() != 3) return 3;

  Derived derived{4, 5, 6};
  if (derived.*pa != 4) return 4;
  if ((derived.*pf)() != 9) return 5;

  return 0;
}
