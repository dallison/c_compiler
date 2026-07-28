// RUN: -std=c++20
// EXPECT: Template argument does not match template parameter list

template <int N>
struct indexed_template {};

template <template <class> class C>
struct holder {};

holder<indexed_template> invalid;
