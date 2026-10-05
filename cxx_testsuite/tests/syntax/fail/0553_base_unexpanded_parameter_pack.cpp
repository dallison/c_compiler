// RUN: -std=c++20
// EXPECT: base specifier contains an unexpanded parameter pack

// A base that names a template parameter pack must expand it with `...`.
template <class T, int I> struct Storage { T v; };
template <class... Ts> struct Tag {};

template <class... Ts> struct Impl : Storage<Ts, 0, Tag<Ts...>> {};
