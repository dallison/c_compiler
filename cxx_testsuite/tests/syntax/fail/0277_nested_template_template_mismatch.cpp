// RUN: -std=c++20
// EXPECT: Template argument does not match template parameter list

template <class T, class U>
struct pair_template {
  int value;
};

template <template <class, class> class Outer>
struct enclosing {
  template <template <class> class Inner>
  struct nested {
    Inner<int> value;
  };

  nested<Outer> invalid;
};

enclosing<pair_template> instantiate;

int use() {
  return instantiate.invalid.value.value;
}
