struct Left {
  int left;
  Left();
  virtual int f(void);
};

Left::Left() {
  left = 13;
}

int Left::f(void) {
  return left;
}

struct Right {
  int right;
  Right();
  virtual int g(void);
};

Right::Right() {
  right = 17;
}

int Right::g(void) {
  return right;
}

struct Derived : public Left, public Right {
  int own;
  Derived();
  int g(void) override;
};

Derived::Derived() {
  own = 19;
}

int Derived::g(void) {
  return left * 100 + right * 10 + own;
}

int call_right(Right* right) {
  return right->g();
}

int main(void) {
  Derived* derived = new Derived;
  Right* right = derived;
  if (call_right(right) != 1489) {
    return 1;
  }
  if (derived->g() != 1489) {
    return 2;
  }
  delete derived;
  return 0;
}
