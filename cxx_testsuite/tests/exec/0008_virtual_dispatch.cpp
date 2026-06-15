struct Base {
  Base();
  virtual int value(void);
};

Base::Base() {
}

int Base::value(void) {
  return 11;
}

struct Derived : public Base {
  Derived();
  int value(void) override;
};

Derived::Derived() {
}

int Derived::value(void) {
  return 31;
}

int read_base(Base* base) {
  return base->value();
}

int main(void) {
  Base* base = new Base;
  if (read_base(base) != 11) {
    return 1;
  }
  delete base;

  Derived* derived = new Derived;
  if (read_base(derived) != 31) {
    return 2;
  }
  if (derived->value() != 31) {
    return 3;
  }
  delete derived;
  return 0;
}
