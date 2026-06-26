// RUN: -std=c++20
// EXPECT: value is a private member of PrivateUsingBase

struct PrivateUsingBase {
 private:
  int value(void);
};

struct BadPrivateUsing : PrivateUsingBase {
 public:
  using PrivateUsingBase::value;
};
