// typeid: polymorphic dynamic type, static type, identity and name().
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
};
Derived::Derived() {}
int Derived::who() { return 2; }

int main() {
  Derived* dp = new Derived;
  Base* bp = dp;

  // Polymorphic form reflects the most-derived dynamic type.
  if (typeid(*bp) != typeid(Derived)) {
    return 1;
  }
  if (typeid(*bp) == typeid(Base)) {
    return 2;
  }

  // typeid pointer-identity: two expressions of the same dynamic type agree.
  Derived* dp2 = new Derived;
  Base* bp2 = dp2;
  if (typeid(*bp) != typeid(*bp2)) {
    return 3;
  }

  // Static form for a non-class type.
  if (typeid(int) == typeid(Derived)) {
    return 4;
  }
  if (typeid(int) != typeid(int)) {
    return 5;
  }

  // name() is non-null.
  if (typeid(Derived).name() == 0) {
    return 6;
  }

  delete dp;
  delete dp2;
  return 0;
}
