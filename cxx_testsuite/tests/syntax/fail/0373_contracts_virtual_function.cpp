// RUN: -std=c++26
// EXPECT: virtual functions cannot have contract assertions

struct base {
  virtual int invalid() pre (true);
};
