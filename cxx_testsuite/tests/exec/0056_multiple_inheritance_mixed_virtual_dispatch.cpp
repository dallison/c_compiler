struct First {
  int first;
  First();
  virtual int f(void);
};

First::First() {
  first = 3;
}

int First::f(void) {
  return first;
}

struct Second {
  int second;
  Second();
  virtual int g(void);
};

Second::Second() {
  second = 5;
}

int Second::g(void) {
  return second;
}

struct Derived : public First, public Second {
  int own;
  Derived();
  int f(void) override;
  int g(void) override;
};

Derived::Derived() {
  own = 7;
}

int Derived::f(void) {
  return first * 10 + own;
}

int Derived::g(void) {
  return second * 10 + own;
}

int call_first(First* first) {
  return first->f();
}

int call_second(Second* second) {
  return second->g();
}

int main(void) {
  Derived* derived = new Derived;
  First* first = derived;
  Second* second = derived;
  if (call_first(first) != 37) {
    return 1;
  }
  if (call_second(second) != 57) {
    return 2;
  }
  if (derived->f() != 37) {
    return 3;
  }
  if (derived->g() != 57) {
    return 4;
  }
  delete derived;
  return 0;
}
