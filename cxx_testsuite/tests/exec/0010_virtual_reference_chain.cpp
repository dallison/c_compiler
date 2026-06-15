struct Base {
  Base();
  virtual int value(void);
};

Base::Base() {
}

int Base::value(void) {
  return 11;
}

struct Mid : public Base {
  Mid();
  int value(void) override;
};

Mid::Mid() {
}

int Mid::value(void) {
  return 22;
}

struct Derived : public Mid {
  Derived();
  int value(void) override;
};

Derived::Derived() {
}

int Derived::value(void) {
  return 33;
}

int read_base_ptr(Base* base) {
  return base->value();
}

int read_base_ref(Base& base) {
  return base.value();
}

int read_mid_ptr(Mid* mid) {
  return mid->value();
}

int main(void) {
  Derived derived;
  if (read_base_ptr(&derived) != 33) {
    return 1;
  }
  if (read_base_ref(derived) != 33) {
    return 2;
  }
  if (read_mid_ptr(&derived) != 33) {
    return 3;
  }

  Mid mid;
  if (read_base_ref(mid) != 22) {
    return 4;
  }
  return 0;
}
