// RUN: -std=c++20
// A concrete template-name argument uses the Itanium source-name encoding
// `3box` in the enclosing function template's argument list.  The surrounding
// spelling retains DaveCC's existing function-template return-type convention.
// EXPECT-ASM: _Z8identityI3boxEi
// EXPECT-ASM: _Z8identityI9box_aliasEi

template <class T>
struct box {};

template <class T>
using box_alias = box<T>;

template <template <class> class C>
int identity(int value) {
  C<int> instance{};
  (void)instance;
  return value;
}

int use() { return identity<box>(42); }
int use_alias() { return identity<box_alias>(42); }
