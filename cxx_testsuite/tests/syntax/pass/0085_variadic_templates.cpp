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

template <class... Ts>
int count_types(Ts... args) {
  (void)0;
  return sizeof...(Ts);
}

int accepts_three(int a, long b, char c);

struct DirectPack {
  DirectPack(int a, long b, char c);
  int sum(void);
};

template <class... Ts>
int forward_values(Ts... args) {
  return accepts_three(args...);
}

template <class... Ts>
int direct_construct_values(Ts... args) {
  DirectPack value(args...);
  return value.sum();
}

template <class... Ts>
int new_construct_values(Ts... args) {
  DirectPack* value = new DirectPack(args...);
  int result = value->sum();
  delete value;
  return result;
}

template <class... Ts>
DirectPack temporary_construct_values(Ts... args) {
  return DirectPack(args...);
}

template <class... Ts>
DirectPack braced_temporary_construct_values(Ts... args) {
  return DirectPack{args...};
}

template <class... Ts>
int fold_sum_values(Ts... args) {
  return (... + args);
}

template <class... Ts>
bool fold_all_values(Ts... args) {
  return (args && ...);
}

template <class... Ts>
int fold_seeded_sum_values(Ts... args) {
  return (0 + ... + args);
}

template <class... Ts>
int fold_seeded_product_values(Ts... args) {
  return (1 * ... * args);
}

template <class... Ts>
int fold_shift_values(int value, Ts... args) {
  return (value << ... << args);
}

template <class... Ts>
int fold_seeded_sum_right_values(Ts... args) {
  return (args + ... + 0);
}

template <class... Ts>
int fold_seeded_product_right_values(Ts... args) {
  return (args * ... * 1);
}

template <class... Ts>
int fold_shift_right_values(Ts... args) {
  return (args << ... << 1);
}

int fold_seed_value(void);

template <class... Ts>
int fold_call_seed_sum_values(Ts... args) {
  return (fold_seed_value() + ... + args);
}

template <class... Ts>
int fold_paren_seed_sum_values(int base, Ts... args) {
  return ((base + 1) + ... + args);
}

template <class... Ts>
int fold_call_seed_sum_right_values(Ts... args) {
  return (args + ... + fold_seed_value());
}

template <class... Ts>
int fold_paren_seed_sum_right_values(int base, Ts... args) {
  return (args + ... + (base + 1));
}

template <class... Ts>
int fold_difference_values(int seed, Ts... args) {
  return (seed - ... - args);
}

template <class... Ts>
int fold_difference_right_values(Ts... args) {
  return (args - ... - 20);
}

template <class... Ts>
int fold_divide_values(int seed, Ts... args) {
  return (seed / ... / args);
}

template <class... Ts>
int fold_mod_values(int seed, Ts... args) {
  return (seed % ... % args);
}

template <class... Ts>
int fold_right_shift_values(int value, Ts... args) {
  return (value >> ... >> args);
}

template <class... Ts>
int fold_bitwise_values(Ts... args) {
  return (255 & ... & args) | (0 | ... | args) | (0 ^ ... ^ args);
}

template <class... Ts>
int braced_values(Ts... args) {
  int values[] = { args... };
  (void)values;
  return sizeof...(Ts);
}

void use_variadic_templates(void) {
  TypePack<> empty_types;
  TypePack<int, long> type_pack;
  IntPack<> empty_ints;
  IntPack<1, 2, 3> int_pack;
  WrappedTuple<int, char, long> wrapped;
  FirstAndRest<int, char, long> split;
  int value = variadic_function(1, 2L, 'c');
  int count = count_types(1, 2L, 'c');
  int forwarded = forward_values(1, 2L, 'c');
  int direct_constructed = direct_construct_values(1, 2L, 'c');
  int new_constructed = new_construct_values(1, 2L, 'c');
  DirectPack temporary_constructed = temporary_construct_values(1, 2L, 'c');
  DirectPack braced_temporary_constructed =
      braced_temporary_construct_values(1, 2L, 'c');
  int folded = fold_sum_values(1, 2, 3);
  bool all = fold_all_values(1, 1, 1);
  int seeded_sum = fold_seeded_sum_values();
  int seeded_product = fold_seeded_product_values();
  int shifted = fold_shift_values(1, 1, 2);
  int seeded_sum_right = fold_seeded_sum_right_values();
  int seeded_product_right = fold_seeded_product_right_values();
  int shifted_right = fold_shift_right_values(1, 2);
  int call_seed = fold_call_seed_sum_values(1, 2);
  int paren_seed = fold_paren_seed_sum_values(10, 1, 2);
  int call_seed_right = fold_call_seed_sum_right_values(1, 2);
  int paren_seed_right = fold_paren_seed_sum_right_values(10, 1, 2);
  int difference = fold_difference_values(20, 3, 4);
  int difference_right = fold_difference_right_values(3, 4);
  int divided = fold_divide_values(100, 2, 5);
  int modded = fold_mod_values(100, 7, 5);
  int right_shifted = fold_right_shift_values(64, 1, 2);
  int bitwise = fold_bitwise_values(240, 51);
  int braced = braced_values(1, 2, 3);
  (void)empty_types;
  (void)type_pack;
  (void)empty_ints;
  (void)int_pack;
  (void)wrapped;
  (void)split;
  (void)value;
  (void)count;
  (void)forwarded;
  (void)direct_constructed;
  (void)new_constructed;
  (void)temporary_constructed;
  (void)braced_temporary_constructed;
  (void)folded;
  (void)all;
  (void)seeded_sum;
  (void)seeded_product;
  (void)shifted;
  (void)seeded_sum_right;
  (void)seeded_product_right;
  (void)shifted_right;
  (void)call_seed;
  (void)paren_seed;
  (void)call_seed_right;
  (void)paren_seed_right;
  (void)difference;
  (void)difference_right;
  (void)divided;
  (void)modded;
  (void)right_shifted;
  (void)bitwise;
  (void)braced;
}
