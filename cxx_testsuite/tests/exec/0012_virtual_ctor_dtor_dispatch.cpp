int trace;

struct Base {
  Base();
  virtual ~Base();
  virtual int value(void);
};

Base::Base() {
  trace = trace * 10 + this->value();
}

Base::~Base() {
  trace = trace * 10 + this->value();
}

int Base::value(void) {
  return 1;
}

struct Derived : public Base {
  Derived();
  ~Derived() override;
  int value(void) override;
};

Derived::Derived() {
  trace = trace * 10 + this->value();
}

Derived::~Derived() {
  trace = trace * 10 + this->value();
}

int Derived::value(void) {
  return 2;
}

int main(void) {
  Base* base = new Derived;
  if (trace != 12) {
    return 1;
  }
  delete base;
  if (trace != 1221) {
    return 2;
  }
  return 0;
}
