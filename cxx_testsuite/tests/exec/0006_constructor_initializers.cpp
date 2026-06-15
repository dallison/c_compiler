int trace;

struct Base {
  int base_value;
  Base(int value);
};

Base::Base(int value) {
  if (trace != 0) {
    trace = 100;
    return;
  }
  trace = 1;
  base_value = value;
}

struct Part {
  int part_value;
  Part(int value);
};

Part::Part(int value) {
  if (trace != 1) {
    trace = 101;
    return;
  }
  trace = 2;
  part_value = value;
}

struct Derived : public Base {
  Part part;
  int derived_value;
  Derived();
};

Derived::Derived() : derived_value(31), part(17), Base(11) {
  if (trace != 2) {
    trace = 102;
    return;
  }
  if (base_value != 11 || part.part_value != 17 || derived_value != 31) {
    trace = 103;
    return;
  }
  trace = 3;
}

int main(void) {
  Derived* derived = new Derived;
  if (trace != 3) {
    return trace;
  }
  if (derived->base_value != 11) {
    return 4;
  }
  if (derived->part.part_value != 17) {
    return 5;
  }
  if (derived->derived_value != 31) {
    return 6;
  }
  delete derived;
  return 0;
}
