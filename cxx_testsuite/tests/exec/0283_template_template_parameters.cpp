// RUN: -std=c++20

template <class T>
struct box {
  T value;
};

template <class T>
struct other_box {
  T value;
};

template <class T, class U = int>
struct box_with_default {
  T first;
  U second;
};

template <class T, class U>
struct pair_box {
  int value;
};

template <template <class, class> class Outer>
struct dependent_nested_holder {
  template <template <class, class> class Inner>
  struct nested {
    Inner<int, long> value;
  };

  nested<Outer> value;
};

template <class T>
using box_alias = box<T>;

template <template <class> class C = box, class T = int>
struct holder {
  C<T> value;
};

template <template <class> class C, class T>
int read_holder(holder<C, T> h) {
  return h.value.value;
}

template <template <class> class C, class T>
T read_direct(const C<T>& value) {
  return value.value;
}

template <class... Ts>
struct pack_box {};

template <template <class...> class C, class... Ts>
struct pack_holder {
  C<Ts...> value;
};

template <template <class> class... Cs>
struct template_pack {};

template <template <class> class Inner>
struct outer {
  Inner<int> value;
};

template <template <template <class> class> class Outer>
struct nested_holder {
  Outer<box> value;
};

template <class... Ts>
struct type_list {};

template <template <class> class... Cs>
struct apply_template_pack {
  type_list<Cs<int>...> value;
};

template <class T>
struct classified {
  int value() { return 0; }
};

template <template <class> class C, class T>
struct classified<C<T>> {
  int value() { return 1; }
};

template <int N>
struct indexed_box {
  int value;
};

template <template <int> class C>
struct indexed_holder {
  C<3> value;
};

template <template <class> class A,
          template <class> class B = A>
struct dependent_default_holder {
  A<int> first;
  B<int> second;
};

int main() {
  holder<box, int> h{};
  h.value.value = 42;
  holder<> by_default{};
  by_default.value.value = 7;
  holder<box_alias, int> by_alias{};
  by_alias.value.value = 9;
  holder<other_box, int> distinct_binding{};
  distinct_binding.value.value = 17;
  holder<box_with_default, int> compatible_default{};
  compatible_default.value.first = 11;
  pack_holder<pack_box, int, long> packed{};
  template_pack<box, box_alias> templates{};
  nested_holder<outer> nested{};
  nested.value.value.value = 13;
  classified<box<int>> classification{};
  classified<pair_box<int, long>> nonclassification{};
  apply_template_pack<box, box_alias> applied{};
  box<int> direct{19};
  indexed_holder<indexed_box> indexed{};
  indexed.value.value = 23;
  dependent_default_holder<box> dependent_default{};
  dependent_default.first.value = 29;
  dependent_default.second.value = 31;
  dependent_nested_holder<pair_box> dependent_nested{};

  if (read_holder(h) != 42) return 1;
  if (by_default.value.value != 7) return 2;
  if (by_alias.value.value != 9) return 3;
  if (distinct_binding.value.value != 17) return 12;
  if (compatible_default.value.first != 11) return 4;
  (void)packed;
  (void)templates;
  if (nested.value.value.value != 13) return 5;
  if (classification.value() != 1) return 6;
  if (nonclassification.value() != 0) return 11;
  if (read_direct(direct) != 19) return 7;
  if (read_direct<box>(direct) != 19) return 8;
  if (indexed.value.value != 23) return 9;
  if (dependent_default.first.value != 29 ||
      dependent_default.second.value != 31) return 10;
  (void)dependent_nested;
  (void)applied;
  return 0;
}
