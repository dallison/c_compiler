// dynamic_cast (reference form): successful downcast returns a usable
// reference; a failing downcast throws std::bad_cast.
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
Derived::Derived() { extra = 55; }
int Derived::who() { return 2; }

int main() {
  Derived* dp = new Derived;
  Base* bp = dp;

  // Successful reference downcast.
  Derived& dr = dynamic_cast<Derived&>(*bp);
  if (dr.extra != 55) {
    return 1;
  }
  if (dr.who() != 2) {
    return 2;
  }

  // Failing reference downcast throws std::bad_cast.
  Base* plain = new Base;
  int caught = 0;
  try {
    Derived& bad = dynamic_cast<Derived&>(*plain);
    (void)bad.extra;
  } catch (std::bad_cast&) {
    caught = 1;
  }
  if (caught != 1) {
    return 3;
  }

  delete dp;
  delete plain;
  return 0;
}
