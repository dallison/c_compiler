// RUN: -std=c++17
// EXPECT: value overrides final function
struct Base {
  virtual int value(void) final;
};

struct Derived : public Base {
  int value(void) override;
};
