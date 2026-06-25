// RUN: -std=c++20
// EXPECT: Static data member initializer requires inline
struct BadStaticDataMember {
  static int value = 1;
};
