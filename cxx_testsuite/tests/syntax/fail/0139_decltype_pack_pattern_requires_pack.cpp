// RUN: -std=c++20
// EXPECT: template argument pack expansion requires a parameter pack

template <class... Ts>
struct Tuple {
};

template <class T>
struct BadDecltypePackPattern {
  Tuple<decltype(T())...> value;
};

BadDecltypePackPattern<int> bad_decltype_pack_pattern;
