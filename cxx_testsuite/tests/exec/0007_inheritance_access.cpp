struct Base {
 public:
  int public_value;
 protected:
  int protected_value;
 private:
  int private_value;
};

struct Derived : public Base {
  int set_values(void);
};

int Derived::set_values(void) {
  public_value = 11;
  protected_value = 31;
  return public_value + protected_value;
}

struct PrivateDerived : private Base {
  int set_private_base(void);
};

int PrivateDerived::set_private_base(void) {
  public_value = 7;
  protected_value = 13;
  return public_value + protected_value;
}

int main(void) {
  Derived derived;
  if (derived.set_values() != 42) {
    return 1;
  }
  if (derived.public_value != 11) {
    return 2;
  }

  PrivateDerived private_derived;
  if (private_derived.set_private_base() != 20) {
    return 3;
  }
  return 0;
}
