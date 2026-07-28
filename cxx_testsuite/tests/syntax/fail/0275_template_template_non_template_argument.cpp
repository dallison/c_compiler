// RUN: -std=c++20
// EXPECT: Template argument must name a compatible template

template <template <class> class C>
struct holder {};

holder<int> invalid;
