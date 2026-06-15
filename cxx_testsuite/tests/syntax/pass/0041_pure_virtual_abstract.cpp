// RUN: -std=c++20

struct Abstract {
  virtual int value(void) = 0;
};

struct Concrete : public Abstract {
  int value(void) override;
};

int Concrete::value(void) {
  return 7;
}

int read_pointer(Abstract* value) {
  return value->value();
}

int read_reference(Abstract& value) {
  return value.value();
}

int main(void) {
  Concrete concrete;
  Abstract* pointer = &concrete;
  Abstract& reference = concrete;
  return read_pointer(pointer) + read_reference(reference);
}
