template <class... Ts>
int count_types(Ts... args) {
  (void)0;
  return sizeof...(Ts);
}

template <class... Ts>
int count_values(Ts... args) {
  (void)0;
  return sizeof...(args);
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

template <class T>
struct Box {
};

template <class... Ts>
using TupleAlias = Tuple<Ts...>;

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

template <>
struct Tuple<Box<int>, Box<long>, Box<char> > {
  int count(void) {
    return 30;
  }
};

template <class... Ts>
int expanded_tuple_count(void) {
  Tuple<Ts...> value;
  return value.count();
}

template <class... Ts>
int aliased_tuple_count(void) {
  TupleAlias<Ts...> value;
  return value.count();
}

template <class... Ts>
int boxed_tuple_count(void) {
  Tuple<Box<Ts>...> value;
  return value.count();
}

int no_args(void) {
  return 11;
}

int sum_three(int a, long b, char c) {
  return a + (int)b + (int)c;
}

int sum_ints(int a, int b, int c) {
  return a + b + c;
}

int add_one(int value) {
  return value + 1;
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

struct PackBaseA {
  int base_a(void) {
    return 7;
  }
};

struct PackBaseB {
  int base_b(void) {
    return 11;
  }
};

struct MemberUsingBase {
  int member_using_value(void) {
    return 13;
  }
};

struct MemberUsingDataPrefix {
  int prefix;
};

struct MemberUsingDataBase {
 protected:
  int member_using_field;
};

struct MemberUsingTypeBase {
  using member_using_type = int;
};

struct MemberUsingDerived : MemberUsingBase {
  using MemberUsingBase::member_using_value;
};

struct MemberUsingDataDerived : MemberUsingDataPrefix, MemberUsingDataBase {
  using MemberUsingDataBase::member_using_field;
};

struct MemberUsingTypeDerived : MemberUsingTypeBase {
  using MemberUsingTypeBase::member_using_type;
};

template <class Base>
struct DependentMemberUsing : Base {
  using Base::member_using_value;
};

template <class Base>
struct DependentDataMemberUsing : MemberUsingDataPrefix, Base {
  using Base::member_using_field;
};

template <class Base>
struct DependentTypeMemberUsing : Base {
  using Base::member_using_type;
};

struct MemberUsingOverloadA {
  int member_using_overload(int value) {
    return value + 17;
  }
};

struct MemberUsingOverloadB {
  int member_using_overload(long value) {
    return (int)value + 19;
  }
};

template <class... Bases>
struct ExpandedBases : Bases... {
  int count(void) {
    return sizeof...(Bases);
  }
};

template <class... Bases>
struct ExpandedMemberUsing : Bases... {
  using Bases::member_using_overload...;

  int count(void) {
    return sizeof...(Bases);
  }
};

template <class... Bases>
struct ExpandedDataMemberUsing : MemberUsingDataPrefix, Bases... {
  using Bases::member_using_field...;
};

template <class... Bases>
struct ExpandedTypeMemberUsing : Bases... {
  using Bases::member_using_type...;
};

int member_using_sum(void) {
  MemberUsingDerived direct;
  MemberUsingDataDerived direct_data;
  DependentMemberUsing<MemberUsingBase> dependent;
  DependentDataMemberUsing<MemberUsingDataBase> dependent_data;
  ExpandedMemberUsing<MemberUsingOverloadA, MemberUsingOverloadB> expanded;
  ExpandedDataMemberUsing<MemberUsingDataBase> expanded_data;
  direct_data.prefix = 5;
  direct_data.member_using_field = 7;
  dependent_data.prefix = 11;
  dependent_data.member_using_field = 13;
  expanded_data.prefix = 17;
  expanded_data.member_using_field = 19;
  return direct.member_using_value() + dependent.member_using_value() +
         expanded.count() + expanded.member_using_overload(1) +
         expanded.member_using_overload(1L) + direct_data.prefix +
         direct_data.member_using_field + dependent_data.prefix +
         dependent_data.member_using_field + expanded_data.prefix +
         expanded_data.member_using_field;
}

int member_using_type_sum(void) {
  typename MemberUsingTypeBase::member_using_type base = 1;
  typename MemberUsingTypeDerived::member_using_type direct = 2;
  typename DependentTypeMemberUsing<MemberUsingTypeBase>::member_using_type
      dependent = 3;
  typename ExpandedTypeMemberUsing<MemberUsingTypeBase>::member_using_type
      expanded = 4;
  return base + direct + dependent + expanded;
}

template <class... Ts>
int forward_no_args(Ts... args) {
  return no_args(args...);
}

template <class... Ts>
int forward_sum(Ts... args) {
  return sum_three(args...);
}

template <class... Ts>
int forward_incremented_sum(Ts... args) {
  return sum_ints((args + 1)...);
}

template <class... Ts>
int forward_mapped_sum(Ts... args) {
  return sum_ints(add_one(args)...);
}

template <class... Ts>
int forward_const_ref_sum(const Ts&... args) {
  return sum_three(args...);
}

template <class... Ts>
int forward_rvalue_ref_sum(Ts&&... args) {
  return sum_three(args...);
}

template <class... Ts>
int boxed_parameter_pack_count(Box<Ts>... boxes) {
  (void)0;
  return sizeof...(boxes);
}

template <class... Ts>
int direct_construct_sum(Ts... args) {
  DirectPack value(args...);
  return value.sum();
}

template <class... Ts>
int direct_construct_incremented_sum(Ts... args) {
  DirectPack value((args + 1)...);
  return value.sum();
}

template <class... Ts>
int direct_construct_mapped_sum(Ts... args) {
  DirectPack value(add_one(args)...);
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
int new_construct_mapped_sum(Ts... args) {
  DirectPack* value = new DirectPack(add_one(args)...);
  int result = value->sum();
  delete value;
  return result;
}

template <class... Ts>
DirectPack temporary_construct_sum(Ts... args) {
  return DirectPack(args...);
}

template <class... Ts>
DirectPack temporary_construct_incremented_sum(Ts... args) {
  return DirectPack((args + 1)...);
}

template <class... Ts>
DirectPack braced_temporary_construct_sum(Ts... args) {
  return DirectPack{args...};
}

template <class... Ts>
DirectPack braced_temporary_construct_incremented_sum(Ts... args) {
  return DirectPack{(args + 1)...};
}

template <class... Ts>
int member_initializer_sum(Ts... args) {
  MemberInitPack<Ts...> value(args...);
  return value.sum();
}

int expanded_base_sum(void) {
  ExpandedBases<PackBaseA, PackBaseB> value;
  return value.count() + value.base_a() + value.base_b();
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

int lambda_capture_pack_call_target(int a, long b, char c) {
  return a + b + c;
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
int fold_mapped_sum_left(Ts... args) {
  return (... + add_one(args));
}

template <class... Ts>
int fold_incremented_difference_right(Ts... args) {
  return ((args + 1) - ...);
}

template <class... Ts>
int fold_seeded_mapped_sum(Ts... args) {
  return (10 + ... + add_one(args));
}

template <class... Ts>
int fold_seeded_incremented_sum_right(Ts... args) {
  return ((args + 1) + ... + 10);
}

template <class... Ts>
int braced_sum(Ts... args) {
  int values[] = { args... };
  return values[0] + values[1] + values[2];
}

template <class... Ts>
int braced_incremented_sum(Ts... args) {
  int values[] = { (args + 1)... };
  return values[0] + values[1] + values[2];
}

template <class... Ts>
int braced_mapped_sum(Ts... args) {
  int values[] = { add_one(args)... };
  return values[0] + values[1] + values[2];
}

template <class... Ts>
int lambda_capture_pack_sum(Ts... args) {
  auto fn = [args...] {
    return (0 + ... + args);
  };
  return fn();
}

template <class... Ts>
int lambda_capture_pack_ref_sum(Ts... args) {
  auto fn = [&args...] {
    return (0 + ... + args);
  };
  return fn();
}

template <class... Ts>
int lambda_capture_pack_call_sum(Ts... args) {
  auto fn = [args...] {
    return lambda_capture_pack_call_target(args...);
  };
  return fn();
}

template <class... Ts>
int lambda_capture_pack_template_call_sum(Ts... args) {
  auto fn = [args...] {
    return forward_sum(args...);
  };
  return fn();
}

template <class... Ts>
int lambda_capture_pack_size(Ts... args) {
  auto fn = [args...] {
    return sizeof...(args);
  };
  return fn();
}

int lambda_init_capture_value(int value) {
  auto fn = [x = value] {
    return x + 1;
  };
  return fn();
}

template <class... Ts>
int lambda_init_capture_pack_sum(Ts... args) {
  auto fn = [xs = args...] {
    return (0 + ... + xs);
  };
  return fn();
}

template <class... Ts>
int lambda_init_capture_pack_call_sum(Ts... args) {
  auto fn = [xs = args...] {
    return lambda_capture_pack_call_target(xs...);
  };
  return fn();
}

template <class... Ts>
int lambda_init_capture_pack_template_call_sum(Ts... args) {
  auto fn = [xs = args...] {
    return forward_sum(xs...);
  };
  return fn();
}

template <class... Ts>
int lambda_init_capture_pack_size(Ts... args) {
  auto fn = [xs = args...] {
    return sizeof...(xs);
  };
  return fn();
}

int main(void) {
  if (count_types() != 0) {
    return 1;
  }
  if (count_types(1, 2L, 'c') != 3) {
    return 2;
  }
  if (count_values() != 0) {
    return 133;
  }
  if (count_values(1, 2L, (char)3) != 3) {
    return 134;
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
  if (forward_incremented_sum(1, 2L, (char)3) != 9) {
    return 112;
  }
  if (forward_mapped_sum(1, 2L, (char)3) != 9) {
    return 113;
  }
  if (forward_const_ref_sum(1, 2L, (char)3) != 6) {
    return 139;
  }
  if (forward_rvalue_ref_sum(1, 2L, (char)3) != 6) {
    return 140;
  }
  Box<int> box_i;
  Box<long> box_l;
  Box<char> box_c;
  if (boxed_parameter_pack_count(box_i, box_l, box_c) != 3) {
    return 141;
  }
  if (expanded_tuple_count() != 0) {
    return 7;
  }
  if (expanded_tuple_count<int, long, char>() != 3) {
    return 8;
  }
  if (aliased_tuple_count() != 0) {
    return 9;
  }
  if (aliased_tuple_count<int, long, char>() != 3) {
    return 10;
  }
  if (boxed_tuple_count<int, long, char>() != 30) {
    return 111;
  }
  if (fold_sum_left(1, 2, 3) != 6) {
    return 11;
  }
  if (fold_sum_right(1, 2, 3) != 6) {
    return 12;
  }
  if (!fold_all()) {
    return 13;
  }
  if (fold_all(1, 0, 1)) {
    return 14;
  }
  if (fold_any()) {
    return 15;
  }
  if (!fold_any(0, 0, 1)) {
    return 16;
  }
  if (fold_seeded_sum() != 0) {
    return 17;
  }
  if (fold_seeded_sum(1, 2, 3) != 6) {
    return 18;
  }
  if (fold_seeded_product() != 1) {
    return 19;
  }
  if (fold_seeded_product(2, 3, 4) != 24) {
    return 20;
  }
  if (fold_shift(1, 1, 2) != 8) {
    return 21;
  }
  if (fold_seeded_sum_right() != 0) {
    return 22;
  }
  if (fold_seeded_sum_right(1, 2, 3) != 6) {
    return 23;
  }
  if (fold_seeded_product_right() != 1) {
    return 24;
  }
  if (fold_seeded_product_right(2, 3, 4) != 24) {
    return 25;
  }
  if (fold_shift_right(1, 2) != 16) {
    return 26;
  }
  if (fold_call_seed_sum(1, 2) != 13) {
    return 27;
  }
  if (fold_paren_seed_sum(10, 1, 2) != 14) {
    return 28;
  }
  if (fold_call_seed_sum_right(1, 2) != 13) {
    return 29;
  }
  if (fold_paren_seed_sum_right(10, 1, 2) != 14) {
    return 30;
  }
  if (fold_shift_paren_seed(1, 1, 2) != 16) {
    return 31;
  }
  if (fold_shift_call_seed_right(1, 2) != 16) {
    return 32;
  }
  if (fold_difference(20, 3, 4) != 13) {
    return 33;
  }
  if (fold_difference_right(3, 4) != 19) {
    return 34;
  }
  if (fold_divide(100, 2, 5) != 10) {
    return 35;
  }
  if (fold_divide_right(100, 5) != 50) {
    return 36;
  }
  if (fold_mod(100, 7, 5) != 2) {
    return 37;
  }
  if (fold_right_shift(64, 1, 2) != 8) {
    return 38;
  }
  if (fold_right_shift_right(64, 2) != 32) {
    return 39;
  }
  if (fold_bitwise_and(240, 51) != 48) {
    return 40;
  }
  if (fold_bitwise_or(1, 4, 8) != 13) {
    return 41;
  }
  if (fold_bitwise_xor(1, 3, 1) != 3) {
    return 42;
  }
  if (fold_mapped_sum_left(1, 2, 3) != 9) {
    return 119;
  }
  if (fold_incremented_difference_right(1, 2, 3) != 3) {
    return 120;
  }
  if (fold_seeded_mapped_sum(1, 2, 3) != 19) {
    return 121;
  }
  if (fold_seeded_incremented_sum_right(1, 2, 3) != 19) {
    return 122;
  }
  if (direct_construct_sum(1, 2L, (char)3) != 6) {
    return 43;
  }
  if (direct_construct_incremented_sum(1, 2L, (char)3) != 9) {
    return 116;
  }
  if (direct_construct_mapped_sum(1, 2L, (char)3) != 9) {
    return 117;
  }
  if (new_construct_sum(1, 2L, (char)3) != 6) {
    return 44;
  }
  if (new_construct_mapped_sum(1, 2L, (char)3) != 9) {
    return 118;
  }
  DirectPack temporary_constructed =
      temporary_construct_sum(1, 2L, (char)3);
  if (temporary_constructed.sum() != 6) {
    return 45;
  }
  DirectPack braced_temporary_constructed =
      braced_temporary_construct_sum(1, 2L, (char)3);
  if (braced_temporary_constructed.sum() != 6) {
    return 46;
  }
  if (member_initializer_sum(1, 2L, (char)3) != 6) {
    return 47;
  }
  if (expanded_base_sum() != 20) {
    return 48;
  }
  if (member_using_sum() != 138) {
    return 49;
  }
  if (member_using_type_sum() != 10) {
    return 50;
  }
  if (braced_sum(4, 5, 6) != 15) {
    return 51;
  }
  if (braced_incremented_sum(4, 5, 6) != 18) {
    return 114;
  }
  if (braced_mapped_sum(4, 5, 6) != 18) {
    return 115;
  }
  if (lambda_capture_pack_sum() != 0) {
    return 123;
  }
  if (lambda_capture_pack_sum(1, 2, 3) != 6) {
    return 124;
  }
  if (lambda_capture_pack_ref_sum(1, 2, 3) != 6) {
    return 125;
  }
  if (lambda_capture_pack_call_sum(1, 2L, (char)3) != 6) {
    return 126;
  }
  if (lambda_capture_pack_template_call_sum(1, 2L, (char)3) != 6) {
    return 127;
  }
  if (lambda_capture_pack_size() != 0) {
    return 135;
  }
  if (lambda_capture_pack_size(1, 2L, (char)3) != 3) {
    return 136;
  }
  if (lambda_init_capture_value(5) != 6) {
    return 128;
  }
  if (lambda_init_capture_pack_sum() != 0) {
    return 129;
  }
  if (lambda_init_capture_pack_sum(1, 2, 3) != 6) {
    return 130;
  }
  if (lambda_init_capture_pack_call_sum(1, 2L, (char)3) != 6) {
    return 131;
  }
  if (lambda_init_capture_pack_template_call_sum(1, 2L, (char)3) != 6) {
    return 132;
  }
  if (lambda_init_capture_pack_size() != 0) {
    return 137;
  }
  if (lambda_init_capture_pack_size(1, 2L, (char)3) != 3) {
    return 138;
  }
  return 0;
}
