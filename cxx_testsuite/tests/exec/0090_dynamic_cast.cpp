// dynamic_cast (pointer form): single-inheritance downcast, null on failure,
// non-virtual multiple-inheritance sidecast, and static upcast.
#include <typeinfo>

struct Base {
  Base();
  virtual ~Base();
  virtual int who();
};
Base::Base() {}
Base::~Base() {}
int Base::who() { return 1; }

struct Derived : public Base {
  Derived();
  int who() override;
  int extra;
};
Derived::Derived() { extra = 99; }
int Derived::who() { return 2; }

struct Other {
  Other();
  virtual ~Other();
  virtual int tag();
  int o;
};
Other::Other() { o = 7; }
Other::~Other() {}
int Other::tag() { return 100; }

struct Multi : public Base, public Other {
  Multi();
  int who() override;
  int tag() override;
};
Multi::Multi() {}
int Multi::who() { return 3; }
int Multi::tag() { return 4; }

int main() {
  Derived* dp0 = new Derived;
  Base* bp = dp0;

  // Successful single-inheritance downcast.
  Derived* dp = dynamic_cast<Derived*>(bp);
  if (dp == 0) {
    return 1;
  }
  if (dp->extra != 99) {
    return 2;
  }

  // Failing downcast (the object is not a Derived) yields null.
  Multi* mp = new Multi;
  Base* mb = mp;
  Derived* bad = dynamic_cast<Derived*>(mb);
  if (bad != 0) {
    return 3;
  }

  // Sidecast across a non-virtual multiple-inheritance hierarchy: from the
  // Base subobject to the sibling Other subobject of the same Multi.
  Other* op = dynamic_cast<Other*>(mb);
  if (op == 0) {
    return 4;
  }
  if (op->tag() != 4) {
    return 5;
  }
  if (op->o != 7) {
    return 6;
  }

  // Static upcast still works through dynamic_cast.
  Base* up = dynamic_cast<Base*>(dp);
  if (up == 0) {
    return 7;
  }
  if (up->who() != 2) {
    return 8;
  }

  // dynamic_cast of a null pointer is null.
  Base* null_base = 0;
  if (dynamic_cast<Derived*>(null_base) != 0) {
    return 9;
  }

  delete dp0;
  delete mp;
  return 0;
}
