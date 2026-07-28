// RUN: -std=c++20
// EXPECT: Template argument does not match template parameter list

template <class T, class U>
struct pair_template {};

template <template <class> class... Cs>
struct holder {};

holder<pair_template> invalid;
