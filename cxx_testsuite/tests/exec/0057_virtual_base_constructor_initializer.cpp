struct VBase {
  int value;
  VBase(int v);
};

VBase::VBase(int v) {
  value = v;
}

struct Left : virtual public VBase {
  int left_seen;
  Left(void);
};

Left::Left(void) : VBase(11) {
  left_seen = value;
}

struct Right : virtual public VBase {
  int right_seen;
  Right(void);
};

Right::Right(void) : VBase(13) {
  right_seen = value;
}

struct Derived : public Left, public Right {
  int derived_seen;
  Derived(void);
};

Derived::Derived(void) : VBase(42) {
  derived_seen = value;
}

int main(void) {
  Derived d;
  if (d.value != 42) {
    return 1;
  }
  if (d.left_seen != 42) {
    return 2;
  }
  if (d.right_seen != 42) {
    return 3;
  }
  if (d.derived_seen != 42) {
    return 4;
  }
  return 0;
}
