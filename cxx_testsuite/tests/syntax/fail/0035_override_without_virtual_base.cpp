// RUN: -std=c++17
// EXPECT: value marked override but does not override
struct Base {
  int value(void);
};

struct Derived : public Base {
  int value(void) override;
};
