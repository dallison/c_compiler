// RUN: -std=c++20
// EXPECT: pack expansion requires a function parameter pack

struct DirectPack {
  DirectPack(int value);
};

template <class T>
DirectPack bad_constructor_pack_pattern(T value) {
  return DirectPack((value + 1)...);
}

DirectPack use_bad_constructor_pack_pattern(void) {
  return bad_constructor_pack_pattern(1);
}
