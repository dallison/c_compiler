// RUN: -std=c++20
struct Nested {
  int nested;
  int nested_add(int x);
};

int Nested::nested_add(int x) {
  return nested + x;
}

struct VBase : public Nested {
  int value;
  int add(int x);
};

int VBase::add(int x) {
  return value + x;
}

struct Left : virtual public VBase {
  int left;
};

struct Right : public virtual VBase {
  int right;
};

struct Derived : public Left, public Right {
  int derived;
  Derived();
};

Derived::Derived() {
  value = 1;
  left = 2;
  right = 3;
  derived = 4;
}

int use_virtual_base(Derived* derived) {
  Left* left = derived;
  Right* right = derived;
  VBase* from_left = left;
  VBase* from_right = right;
  Nested* nested = right;
  from_left->value = 10;
  nested->nested = 3;
  return from_right->add(derived->value) + left->nested_add(1);
}
