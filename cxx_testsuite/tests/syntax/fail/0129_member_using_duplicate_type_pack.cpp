// RUN: -std=c++20
// EXPECT: Duplicate class member value_type

struct TypeUsingA {
  using value_type = int;
};

struct TypeUsingB {
  using value_type = long;
};

template <class... Bases>
struct BadTypeUsingPack : Bases... {
  using Bases::value_type...;
};

BadTypeUsingPack<TypeUsingA, TypeUsingB> bad_type_using_pack;
