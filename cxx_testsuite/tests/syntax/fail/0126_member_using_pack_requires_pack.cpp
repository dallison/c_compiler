// RUN: -std=c++20
// EXPECT: member using pack expansion requires a template parameter pack

struct Base {
  int value(void);
};

template <class T>
struct BadMemberUsingExpansion : T {
  using T::value...;
};
