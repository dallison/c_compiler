// RUN: -std=c++20
// EXPECT_EXIT: 0

struct Base {
  virtual int value() const { return 1; }
};

struct Derived : Base {
  int value() const override { return 7; }
};

int main() {
  Derived derived;
  Base& ref = derived;
  if (ref.value() != 7) {
    return 1;
  }
  const Base& cref = derived;
  if (cref.value() != 7) {
    return 2;
  }
  return 0;
}
