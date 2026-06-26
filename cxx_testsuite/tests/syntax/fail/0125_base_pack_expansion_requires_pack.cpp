// RUN: -std=c++20
// EXPECT: base class pack expansion requires a template parameter pack

struct Base {
};

template <class T>
struct BadBaseExpansion : T... {
};
