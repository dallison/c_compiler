// RUN: -std=c++20

template <class... Ts>
struct TypePack {
};

template <int... Ns>
struct IntPack {
};

template <class... Ts>
struct Tuple {
};

template <class... Ts>
struct WrappedTuple {
  Tuple<Ts...> tuple;
};

template <class T, class... Rest>
struct FirstAndRest {
  T first;
  Tuple<Rest...> rest;
};

template <class... Ts>
int variadic_function(Ts... args) {
  (void)0;
  return 17;
}

void use_variadic_templates(void) {
  TypePack<> empty_types;
  TypePack<int, long> type_pack;
  IntPack<> empty_ints;
  IntPack<1, 2, 3> int_pack;
  WrappedTuple<int, char, long> wrapped;
  FirstAndRest<int, char, long> split;
  int value = variadic_function(1, 2L, 'c');
  (void)empty_types;
  (void)type_pack;
  (void)empty_ints;
  (void)int_pack;
  (void)wrapped;
  (void)split;
  (void)value;
}
