template <class... Ts>
int count_types(Ts... args) {
  (void)0;
  return sizeof...(Ts);
}

template <class... Ts>
struct TypeCounter {
  int count(void) {
    return sizeof...(Ts);
  }
};

template <class... Ts>
struct Tuple {
};

template <>
struct Tuple<> {
  int count(void) {
    return 0;
  }
};

template <>
struct Tuple<int, long, char> {
  int count(void) {
    return 3;
  }
};

template <class... Ts>
int expanded_tuple_count(void) {
  Tuple<Ts...> value;
  return value.count();
}

int no_args(void) {
  return 11;
}

int sum_three(int a, long b, char c) {
  return a + (int)b + (int)c;
}

struct DirectPack {
  int total;

  DirectPack(int a, long b, char c) {
    total = a + (int)b + (int)c;
  }

  int sum(void) {
    return total;
  }
};

template <class... Ts>
struct MemberInitPack {
  DirectPack value;

  MemberInitPack(Ts... args) : value(args...) {
  }

  int sum(void) {
    return value.sum();
  }
};

template <class... Ts>
int forward_no_args(Ts... args) {
  return no_args(args...);
}

template <class... Ts>
int forward_sum(Ts... args) {
  return sum_three(args...);
}

template <class... Ts>
int direct_construct_sum(Ts... args) {
  DirectPack value(args...);
  return value.sum();
}

template <class... Ts>
int new_construct_sum(Ts... args) {
  DirectPack* value = new DirectPack(args...);
  int result = value->sum();
  delete value;
  return result;
}

template <class... Ts>
DirectPack temporary_construct_sum(Ts... args) {
  return DirectPack(args...);
}

template <class... Ts>
DirectPack braced_temporary_construct_sum(Ts... args) {
  return DirectPack{args...};
}

template <class... Ts>
int member_initializer_sum(Ts... args) {
  MemberInitPack<Ts...> value(args...);
  return value.sum();
}

template <class... Ts>
int fold_sum_left(Ts... args) {
  return (... + args);
}

template <class... Ts>
int fold_sum_right(Ts... args) {
  return (args + ...);
}

template <class... Ts>
bool fold_all(Ts... args) {
  return (... && args);
}

template <class... Ts>
bool fold_any(Ts... args) {
  return (... || args);
}

template <class... Ts>
int fold_seeded_sum(Ts... args) {
  return (0 + ... + args);
}

template <class... Ts>
int fold_seeded_product(Ts... args) {
  return (1 * ... * args);
}

template <class... Ts>
int fold_shift(int value, Ts... args) {
  return (value << ... << args);
}

template <class... Ts>
int fold_seeded_sum_right(Ts... args) {
  return (args + ... + 0);
}

template <class... Ts>
int fold_seeded_product_right(Ts... args) {
  return (args * ... * 1);
}

template <class... Ts>
int fold_shift_right(Ts... args) {
  return (args << ... << 1);
}

int fold_seed_value(void) {
  return 10;
}

int fold_seed_one(void) {
  return 1;
}

template <class... Ts>
int fold_call_seed_sum(Ts... args) {
  return (fold_seed_value() + ... + args);
}

template <class... Ts>
int fold_paren_seed_sum(int base, Ts... args) {
  return ((base + 1) + ... + args);
}

template <class... Ts>
int fold_call_seed_sum_right(Ts... args) {
  return (args + ... + fold_seed_value());
}

template <class... Ts>
int fold_paren_seed_sum_right(int base, Ts... args) {
  return (args + ... + (base + 1));
}

template <class... Ts>
int fold_shift_paren_seed(int value, Ts... args) {
  return ((value + 1) << ... << args);
}

template <class... Ts>
int fold_shift_call_seed_right(Ts... args) {
  return (args << ... << fold_seed_one());
}

template <class... Ts>
int fold_difference(int seed, Ts... args) {
  return (seed - ... - args);
}

template <class... Ts>
int fold_difference_right(Ts... args) {
  return (args - ... - 20);
}

template <class... Ts>
int fold_divide(int seed, Ts... args) {
  return (seed / ... / args);
}

template <class... Ts>
int fold_divide_right(Ts... args) {
  return (args / ... / 2);
}

template <class... Ts>
int fold_mod(int seed, Ts... args) {
  return (seed % ... % args);
}

template <class... Ts>
int fold_right_shift(int value, Ts... args) {
  return (value >> ... >> args);
}

template <class... Ts>
int fold_right_shift_right(Ts... args) {
  return (args >> ... >> 1);
}

template <class... Ts>
int fold_bitwise_and(Ts... args) {
  return (255 & ... & args);
}

template <class... Ts>
int fold_bitwise_or(Ts... args) {
  return (0 | ... | args);
}

template <class... Ts>
int fold_bitwise_xor(Ts... args) {
  return (0 ^ ... ^ args);
}

template <class... Ts>
int braced_sum(Ts... args) {
  int values[] = { args... };
  return values[0] + values[1] + values[2];
}

int main(void) {
  if (count_types() != 0) {
    return 1;
  }
  if (count_types(1, 2L, 'c') != 3) {
    return 2;
  }

  TypeCounter<> empty;
  TypeCounter<int, char, long> three;
  if (empty.count() != 0) {
    return 3;
  }
  if (three.count() != 3) {
    return 4;
  }
  if (forward_no_args() != 11) {
    return 5;
  }
  if (forward_sum(1, 2L, (char)3) != 6) {
    return 6;
  }
  if (expanded_tuple_count() != 0) {
    return 7;
  }
  if (expanded_tuple_count<int, long, char>() != 3) {
    return 8;
  }
  if (fold_sum_left(1, 2, 3) != 6) {
    return 9;
  }
  if (fold_sum_right(1, 2, 3) != 6) {
    return 10;
  }
  if (!fold_all()) {
    return 11;
  }
  if (fold_all(1, 0, 1)) {
    return 12;
  }
  if (fold_any()) {
    return 13;
  }
  if (!fold_any(0, 0, 1)) {
    return 14;
  }
  if (fold_seeded_sum() != 0) {
    return 15;
  }
  if (fold_seeded_sum(1, 2, 3) != 6) {
    return 16;
  }
  if (fold_seeded_product() != 1) {
    return 17;
  }
  if (fold_seeded_product(2, 3, 4) != 24) {
    return 18;
  }
  if (fold_shift(1, 1, 2) != 8) {
    return 19;
  }
  if (fold_seeded_sum_right() != 0) {
    return 20;
  }
  if (fold_seeded_sum_right(1, 2, 3) != 6) {
    return 21;
  }
  if (fold_seeded_product_right() != 1) {
    return 22;
  }
  if (fold_seeded_product_right(2, 3, 4) != 24) {
    return 23;
  }
  if (fold_shift_right(1, 2) != 16) {
    return 24;
  }
  if (fold_call_seed_sum(1, 2) != 13) {
    return 25;
  }
  if (fold_paren_seed_sum(10, 1, 2) != 14) {
    return 26;
  }
  if (fold_call_seed_sum_right(1, 2) != 13) {
    return 27;
  }
  if (fold_paren_seed_sum_right(10, 1, 2) != 14) {
    return 28;
  }
  if (fold_shift_paren_seed(1, 1, 2) != 16) {
    return 29;
  }
  if (fold_shift_call_seed_right(1, 2) != 16) {
    return 30;
  }
  if (fold_difference(20, 3, 4) != 13) {
    return 31;
  }
  if (fold_difference_right(3, 4) != 19) {
    return 32;
  }
  if (fold_divide(100, 2, 5) != 10) {
    return 33;
  }
  if (fold_divide_right(100, 5) != 50) {
    return 34;
  }
  if (fold_mod(100, 7, 5) != 2) {
    return 35;
  }
  if (fold_right_shift(64, 1, 2) != 8) {
    return 36;
  }
  if (fold_right_shift_right(64, 2) != 32) {
    return 37;
  }
  if (fold_bitwise_and(240, 51) != 48) {
    return 38;
  }
  if (fold_bitwise_or(1, 4, 8) != 13) {
    return 39;
  }
  if (fold_bitwise_xor(1, 3, 1) != 3) {
    return 40;
  }
  if (direct_construct_sum(1, 2L, (char)3) != 6) {
    return 41;
  }
  if (new_construct_sum(1, 2L, (char)3) != 6) {
    return 42;
  }
  DirectPack temporary_constructed =
      temporary_construct_sum(1, 2L, (char)3);
  if (temporary_constructed.sum() != 6) {
    return 43;
  }
  DirectPack braced_temporary_constructed =
      braced_temporary_construct_sum(1, 2L, (char)3);
  if (braced_temporary_constructed.sum() != 6) {
    return 44;
  }
  if (member_initializer_sum(1, 2L, (char)3) != 6) {
    return 45;
  }
  if (braced_sum(4, 5, 6) != 15) {
    return 46;
  }
  return 0;
}
