// RUN: -std=c++17
struct Base {
  virtual int value(void);
  virtual int other(int x) const;
  virtual ~Base();
};

int Base::value(void) {
  return 1;
}

int Base::other(int x) const {
  return x;
}

Base::~Base() {
}

struct Derived : public Base {
  int value(void) override;
  int other(int x) const override final;
  ~Derived() override;
};

int Derived::value(void) {
  return 2;
}

int Derived::other(int x) const {
  return x + 1;
}

Derived::~Derived() {
}

int main(void) {
  Derived derived;
  return derived.value() + derived.other(3);
}
