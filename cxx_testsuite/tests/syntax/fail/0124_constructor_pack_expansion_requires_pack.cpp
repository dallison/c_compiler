// RUN: -std=c++20
// EXPECT: pack expansion requires a function parameter pack

struct DirectPack {
  DirectPack(int value);
};

template <class T>
int bad_direct_initializer_pack_expansion(T value) {
  DirectPack direct(value...);
  return 0;
}
