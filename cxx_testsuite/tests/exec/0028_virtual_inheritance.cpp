int trace;

struct Nested {
  int nested;
  int nested_add(int x);
};

int Nested::nested_add(int x) {
  return nested + x;
}

struct VBase : public Nested {
  int value;
  VBase();
  ~VBase();
  int add(int x);
};

VBase::VBase() {
  if (trace != 0) {
    trace = 100;
    return;
  }
  trace = 1;
  nested = 3;
  value = 5;
}

VBase::~VBase() {
  if (trace != 7) {
    trace = 200;
    return;
  }
  trace = 32;
}

int VBase::add(int x) {
  return value + x;
}

struct Left : virtual public VBase {
  int left;
  Left();
  ~Left();
};

Left::Left() {
  if (trace != 1 || value != 5 || nested != 3) {
    trace = 102;
    return;
  }
  nested = 13;
  value = 17;
  trace = 2;
}

Left::~Left() {
  if (trace != 6 || value != 23) {
    trace = 202;
    return;
  }
  trace = 7;
}

struct Right : public virtual VBase {
  int right;
  Right();
  ~Right();
};

Right::Right() {
  if (trace != 2 || value != 17 || nested != 13) {
    trace = 103;
    return;
  }
  nested = 15;
  value = 19;
  trace = 3;
}

Right::~Right() {
  if (trace != 5 || value != 23) {
    trace = 203;
    return;
  }
  trace = 6;
}

struct Derived : public Left, public Right {
  int derived;
  Derived();
  ~Derived();
};

Derived::Derived() {
  if (trace != 3 || value != 19 || nested != 15) {
    trace = 104;
    return;
  }
  left = 7;
  right = 11;
  derived = 13;
  trace = 4;
}

Derived::~Derived() {
  if (trace != 4 || value != 23) {
    trace = 204;
    return;
  }
  trace = 5;
}

int main(void) {
  Derived* derived = new Derived;
  if (trace != 4) {
    return trace;
  }

  Left* left = derived;
  Right* right = derived;
  VBase* from_left = left;
  VBase* from_right = right;
  VBase* from_derived = derived;
  Nested* nested_from_left = left;
  Nested* nested_from_right = right;

  from_left->value = 21;
  if (from_right->value != 21) {
    return 1;
  }
  right->value = 23;
  if (left->value != 23) {
    return 2;
  }
  if (from_derived->add(4) != 27) {
    return 3;
  }
  nested_from_left->nested = 31;
  if (nested_from_right->nested_add(6) != 37) {
    return 4;
  }
  if (derived->left != 7 || derived->right != 11 || derived->derived != 13) {
    return 5;
  }

  delete derived;
  if (trace != 32) {
    return trace;
  }
  return 0;
}
