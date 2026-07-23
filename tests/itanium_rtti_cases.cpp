// Itanium RTTI regression: canonical _ZTS/_ZTI symbols, typeid identity, and
// dynamic_cast for single- and multiple-inheritance hierarchies.
#include <typeinfo>

struct Base {
  Base();
  virtual ~Base();
  virtual int tag();
};
Base::Base() {}
Base::~Base() {}
int Base::tag() { return 1; }

struct Derived : public Base {
  Derived();
  int tag() override;
  int extra;
};
Derived::Derived() { extra = 42; }
int Derived::tag() { return 2; }

struct Side {
  Side();
  virtual ~Side();
  virtual int side_tag();
  int value;
};
Side::Side() { value = 7; }
Side::~Side() {}
int Side::side_tag() { return 100; }

struct Multi : public Base, public Side {
  Multi();
  int tag() override;
  int side_tag() override;
};
Multi::Multi() {}
int Multi::tag() { return 3; }
int Multi::side_tag() { return 4; }

int main() {
  Derived* dp = new Derived;
  Base* bp = dp;

  if (typeid(*bp) != typeid(Derived)) {
    return 1;
  }
  if (typeid(Derived).name() == 0 || typeid(Derived).name()[0] == '\0') {
    return 2;
  }

  Derived* down = dynamic_cast<Derived*>(bp);
  if (down == 0 || down->extra != 42) {
    return 3;
  }

  Multi* mp = new Multi;
  Base* mb = mp;
  if (dynamic_cast<Derived*>(mb) != 0) {
    return 4;
  }

  Side* side = dynamic_cast<Side*>(mb);
  if (side == 0 || side->side_tag() != 4 || side->value != 7) {
    return 5;
  }

  delete dp;
  delete mp;
  return 0;
}
