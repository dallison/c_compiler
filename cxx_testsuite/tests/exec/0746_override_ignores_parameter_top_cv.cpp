// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Value {
  int number;
};

struct Base {
  virtual int read(Value value) = 0;
  virtual int pointer(int* value) = 0;
};

struct Derived : Base {
  int read(const Value value) override { return value.number; }
  int pointer(int* const value) override { return *value; }
};

int main() {
  Derived derived;
  Base& base = derived;
  Value value{17};
  int number = 23;
  return base.read(value) == 17 && base.pointer(&number) == 23 ? 0 : 1;
}
