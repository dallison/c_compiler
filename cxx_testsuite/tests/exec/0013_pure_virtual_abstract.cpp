struct Abstract {
  Abstract();
  virtual int value(void) = 0;
};

Abstract::Abstract() {
}

struct Concrete : public Abstract {
  Concrete();
  int value(void) override;
};

Concrete::Concrete() {
}

int Concrete::value(void) {
  return 42;
}

int read_pointer(Abstract* value) {
  return value->value();
}

int read_reference(Abstract& value) {
  return value.value();
}

int main(void) {
  Concrete concrete;
  if (read_pointer(&concrete) != 42) {
    return 1;
  }
  if (read_reference(concrete) != 42) {
    return 2;
  }
  return 0;
}
