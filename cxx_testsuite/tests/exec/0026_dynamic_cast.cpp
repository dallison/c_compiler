struct Base {
  int value;
};

struct Derived : public Base {
  int extra;
};

int main(void) {
  Derived derived;
  derived.value = 17;
  derived.extra = 23;

  Derived* derived_ptr = &derived;
  Base* base_ptr = dynamic_cast<Base*>(derived_ptr);
  if (base_ptr->value != 17) {
    return 1;
  }

  Derived* same_ptr = dynamic_cast<Derived*>(derived_ptr);
  if (same_ptr->extra != 23) {
    return 2;
  }

  return 0;
}
