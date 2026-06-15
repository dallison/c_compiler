struct Base {
  int value;
};

struct Derived : public Base {
  int extra;
};

int read_base(Base* base) {
  return base->value;
}

int main(void) {
  Derived derived;
  derived.value = 11;
  derived.extra = 31;
  if (sizeof(Derived) != sizeof(Base) + sizeof(int)) {
    return 1;
  }
  if (derived.value != 11) {
    return 2;
  }
  if (derived.extra != 31) {
    return 3;
  }
  if (read_base(&derived) != 11) {
    return 4;
  }
  return 0;
}
