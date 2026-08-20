// RUN: -std=c++26
// EXPECT: virtual functions cannot have contract assertions

struct base {
  virtual int value(const int input);
};

struct derived : base {
  int value(const int input) override pre (input >= 0);
};
