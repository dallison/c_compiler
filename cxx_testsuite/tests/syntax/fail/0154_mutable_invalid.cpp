// RUN: -std=c++20
// 'mutable' applies only to non-static, non-const, non-reference data members.
// EXPECT: 'mutable' cannot be applied to a const member
// EXPECT: member cannot be declared both 'mutable' and 'static'
// EXPECT: 'mutable' cannot be applied to a reference member
// EXPECT: 'mutable' can only be applied to data members

struct ConstField {
  mutable const int x;
};

struct StaticField {
  mutable static int y;
};

struct ReferenceField {
  mutable int& r;
};

struct MemberFunction {
  mutable int f(void);
};
