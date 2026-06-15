int trace;

struct Base {
  int base_value;
  Base();
  ~Base();
};

Base::Base() {
  if (trace != 0) {
    trace = 100;
    return;
  }
  trace = 1;
  base_value = 11;
}

Base::~Base() {
  if (trace != 3) {
    trace = 200;
    return;
  }
  trace = 4;
}

struct Derived : public Base {
  int derived_value;
  Derived();
  ~Derived();
};

Derived::Derived() {
  if (trace != 1) {
    trace = 101;
    return;
  }
  trace = 2;
  derived_value = 31;
}

Derived::~Derived() {
  if (trace != 2) {
    trace = 201;
    return;
  }
  trace = 3;
}

int main(void) {
  Derived* derived = new Derived;
  if (trace != 2) {
    return trace;
  }
  if (derived->base_value != 11) {
    return 5;
  }
  if (derived->derived_value != 31) {
    return 6;
  }
  delete derived;
  if (trace != 4) {
    return trace;
  }
  return 0;
}
