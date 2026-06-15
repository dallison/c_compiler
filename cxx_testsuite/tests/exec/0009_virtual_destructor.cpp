int trace;

struct Base {
  Base();
  virtual ~Base();
};

Base::Base() {
  trace = trace * 10 + 1;
}

Base::~Base() {
  trace = trace * 10 + 2;
}

struct Derived : public Base {
  Derived();
  ~Derived() override;
};

Derived::Derived() {
  trace = trace * 10 + 3;
}

Derived::~Derived() {
  trace = trace * 10 + 4;
}

int main(void) {
  Base* base = new Derived;
  if (trace != 13) {
    return 1;
  }
  delete base;
  if (trace != 1342) {
    return 2;
  }
  return 0;
}
