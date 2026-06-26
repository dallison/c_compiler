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

template <class T>
struct Box {
};

template <class... Ts>
using TupleAlias = Tuple<Ts...>;

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
struct BoxedTuple {
  Tuple<Box<Ts>...> tuple;
};

template <class... Ts>
struct DecltypeTuple {
  Tuple<decltype(static_cast<Ts&&>(Ts()))...> tuple;
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

template <class... Ts>
int count_values(Ts... args) {
  (void)0;
  return sizeof...(args);
}

int accepts_three(int a, long b, char c);
int accepts_ints(int a, int b, int c);
int add_one(int value);

struct DirectPack {
  DirectPack(int a, long b, char c);
  int sum(void);
};

template <class... Ts>
struct MemberInitPack {
  DirectPack value;

  MemberInitPack(Ts... args) : value(args...) {
  }

  int sum(void);
};

struct PackBaseA {
  int base_a(void);
};

struct PackBaseB {
  int base_b(void);
};

struct MemberUsingBase {
  int member_using_value(void);
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
  int member_using_overload(int value);
};

struct MemberUsingOverloadB {
  int member_using_overload(long value);
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

template <class... Ts>
int aliased_tuple_values(void) {
  TupleAlias<Ts...> value;
  (void)value;
  return sizeof...(Ts);
}

template <class... Ts>
int forward_values(Ts... args) {
  return accepts_three(args...);
}

template <class... Ts>
int forward_incremented_values(Ts... args) {
  return accepts_ints((args + 1)...);
}

template <class... Ts>
int forward_mapped_values(Ts... args) {
  return accepts_ints(add_one(args)...);
}

template <class... Ts>
int forward_const_ref_values(const Ts&... args) {
  return accepts_three(args...);
}

template <class... Ts>
int forward_rvalue_ref_values(Ts&&... args) {
  return accepts_three(args...);
}

template <class... Ts>
int forward_cast_values(Ts&&... args) {
  return accepts_three(static_cast<Ts&&>(args)...);
}

template <class... Ts>
int boxed_parameter_pack_values(Box<Ts>... boxes) {
  (void)0;
  return sizeof...(boxes);
}

template <class... Ts>
int decltype_tuple_values(Ts... args) {
  Tuple<decltype(args)...> value;
  (void)value;
  return sizeof...(args);
}

template <class... Ts>
int decltype_forward_tuple_values(Ts&&... args) {
  Tuple<decltype(static_cast<Ts&&>(args))...> value;
  (void)value;
  return sizeof...(args);
}

template <class... Ts>
int direct_construct_values(Ts... args) {
  DirectPack value(args...);
  return value.sum();
}

template <class... Ts>
int direct_construct_incremented_values(Ts... args) {
  DirectPack value((args + 1)...);
  return value.sum();
}

template <class... Ts>
int direct_construct_mapped_values(Ts... args) {
  DirectPack value(add_one(args)...);
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
int new_construct_mapped_values(Ts... args) {
  DirectPack* value = new DirectPack(add_one(args)...);
  int result = value->sum();
  delete value;
  return result;
}

template <class... Ts>
DirectPack temporary_construct_values(Ts... args) {
  return DirectPack(args...);
}

template <class... Ts>
DirectPack temporary_construct_incremented_values(Ts... args) {
  return DirectPack((args + 1)...);
}

template <class... Ts>
DirectPack braced_temporary_construct_values(Ts... args) {
  return DirectPack{args...};
}

template <class... Ts>
DirectPack braced_temporary_construct_incremented_values(Ts... args) {
  return DirectPack{(args + 1)...};
}

template <class... Ts>
int member_initializer_values(Ts... args) {
  MemberInitPack<Ts...> value(args...);
  return value.sum();
}

int expanded_base_values(void) {
  ExpandedBases<PackBaseA, PackBaseB> value;
  return value.count() + value.base_a() + value.base_b();
}

int member_using_values(void) {
  MemberUsingDerived direct;
  MemberUsingDataDerived direct_data;
  DependentMemberUsing<MemberUsingBase> dependent;
  DependentDataMemberUsing<MemberUsingDataBase> dependent_data;
  ExpandedMemberUsing<MemberUsingOverloadA, MemberUsingOverloadB> expanded;
  ExpandedDataMemberUsing<MemberUsingDataBase> expanded_data;
  direct_data.member_using_field = 1;
  dependent_data.member_using_field = 2;
  expanded_data.member_using_field = 3;
  return direct.member_using_value() + dependent.member_using_value() +
         expanded.count() + expanded.member_using_overload(1) +
         expanded.member_using_overload(1L) +
         direct_data.member_using_field + dependent_data.member_using_field +
         expanded_data.member_using_field;
}

int member_using_type_values(void) {
  typename MemberUsingTypeBase::member_using_type base = 1;
  typename MemberUsingTypeDerived::member_using_type direct = 2;
  typename DependentTypeMemberUsing<MemberUsingTypeBase>::member_using_type
      dependent = 3;
  typename ExpandedTypeMemberUsing<MemberUsingTypeBase>::member_using_type
      expanded = 4;
  return base + direct + dependent + expanded;
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
int lambda_capture_pack_call_target(int a, long b, char c);

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
int fold_mapped_sum_left_values(Ts... args) {
  return (... + add_one(args));
}

template <class... Ts>
int fold_incremented_difference_right_values(Ts... args) {
  return ((args + 1) - ...);
}

template <class... Ts>
int fold_seeded_mapped_sum_values(Ts... args) {
  return (10 + ... + add_one(args));
}

template <class... Ts>
int fold_seeded_incremented_sum_right_values(Ts... args) {
  return ((args + 1) + ... + 10);
}

template <class... Ts>
int braced_values(Ts... args) {
  int values[] = { args... };
  (void)values;
  return sizeof...(Ts);
}

template <class... Ts>
int braced_incremented_values(Ts... args) {
  int values[] = { (args + 1)... };
  (void)values;
  return sizeof...(Ts);
}

template <class... Ts>
int braced_mapped_values(Ts... args) {
  int values[] = { add_one(args)... };
  (void)values;
  return sizeof...(Ts);
}

template <class... Ts>
int lambda_capture_pack_values(Ts... args) {
  auto fn = [args...] {
    return (0 + ... + args);
  };
  return fn();
}

template <class... Ts>
int lambda_capture_pack_ref_values(Ts... args) {
  auto fn = [&args...] {
    return (0 + ... + args);
  };
  return fn();
}

template <class... Ts>
int lambda_capture_pack_call_values(Ts... args) {
  auto fn = [args...] {
    return lambda_capture_pack_call_target(args...);
  };
  return fn();
}

template <class... Ts>
int lambda_capture_pack_template_call_values(Ts... args) {
  auto fn = [args...] {
    return forward_values(args...);
  };
  return fn();
}

template <class... Ts>
int lambda_capture_pack_size_values(Ts... args) {
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
int lambda_init_capture_pack_values(Ts... args) {
  auto fn = [xs = args...] {
    return (0 + ... + xs);
  };
  return fn();
}

template <class... Ts>
int lambda_init_capture_pack_call_values(Ts... args) {
  auto fn = [xs = args...] {
    return lambda_capture_pack_call_target(xs...);
  };
  return fn();
}

template <class... Ts>
int lambda_init_capture_pack_template_call_values(Ts... args) {
  auto fn = [xs = args...] {
    return forward_values(xs...);
  };
  return fn();
}

template <class... Ts>
int lambda_init_capture_pack_size_values(Ts... args) {
  auto fn = [xs = args...] {
    return sizeof...(xs);
  };
  return fn();
}

void use_variadic_templates(void) {
  TypePack<> empty_types;
  TypePack<int, long> type_pack;
  IntPack<> empty_ints;
  IntPack<1, 2, 3> int_pack;
  TupleAlias<int, char, long> alias_tuple;
  WrappedTuple<int, char, long> wrapped;
  FirstAndRest<int, char, long> split;
  BoxedTuple<int, char, long> boxed;
  DecltypeTuple<int, char, long> decltype_tuple;
  int value = variadic_function(1, 2L, 'c');
  int count = count_types(1, 2L, 'c');
  int value_count = count_values(1, 2L, 'c');
  int alias_count = aliased_tuple_values<int, char, long>();
  int forwarded = forward_values(1, 2L, 'c');
  int forwarded_incremented = forward_incremented_values(1, 2L, 'c');
  int forwarded_mapped = forward_mapped_values(1, 2L, 'c');
  int forwarded_const_ref = forward_const_ref_values(1, 2L, 'c');
  int forwarded_rvalue_ref = forward_rvalue_ref_values(1, 2L, 'c');
  int forwarded_cast = forward_cast_values(1, 2L, 'c');
  Box<int> box_i;
  Box<long> box_l;
  Box<char> box_c;
  int boxed_parameters = boxed_parameter_pack_values(box_i, box_l, box_c);
  int decltype_parameters = decltype_tuple_values(1, 2L, 'c');
  int decltype_forward_parameters =
      decltype_forward_tuple_values(1, 2L, 'c');
  int direct_constructed = direct_construct_values(1, 2L, 'c');
  int direct_constructed_incremented =
      direct_construct_incremented_values(1, 2L, 'c');
  int direct_constructed_mapped =
      direct_construct_mapped_values(1, 2L, 'c');
  int new_constructed = new_construct_values(1, 2L, 'c');
  int new_constructed_mapped = new_construct_mapped_values(1, 2L, 'c');
  DirectPack temporary_constructed = temporary_construct_values(1, 2L, 'c');
  DirectPack temporary_constructed_incremented =
      temporary_construct_incremented_values(1, 2L, 'c');
  DirectPack braced_temporary_constructed =
      braced_temporary_construct_values(1, 2L, 'c');
  DirectPack braced_temporary_constructed_incremented =
      braced_temporary_construct_incremented_values(1, 2L, 'c');
  int member_initialized = member_initializer_values(1, 2L, 'c');
  int expanded_bases = expanded_base_values();
  int member_using = member_using_values();
  int member_using_type = member_using_type_values();
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
  int mapped_fold = fold_mapped_sum_left_values(1, 2, 3);
  int incremented_difference =
      fold_incremented_difference_right_values(1, 2, 3);
  int seeded_mapped_fold = fold_seeded_mapped_sum_values(1, 2, 3);
  int seeded_incremented_fold =
      fold_seeded_incremented_sum_right_values(1, 2, 3);
  int braced = braced_values(1, 2, 3);
  int braced_incremented = braced_incremented_values(1, 2, 3);
  int braced_mapped = braced_mapped_values(1, 2, 3);
  int lambda_captured = lambda_capture_pack_values(1, 2, 3);
  int lambda_captured_empty = lambda_capture_pack_values();
  int lambda_captured_ref = lambda_capture_pack_ref_values(1, 2, 3);
  int lambda_captured_call =
      lambda_capture_pack_call_values(1, 2L, 'c');
  int lambda_captured_template_call =
      lambda_capture_pack_template_call_values(1, 2L, 'c');
  int lambda_captured_size = lambda_capture_pack_size_values(1, 2L, 'c');
  int lambda_init_captured = lambda_init_capture_value(5);
  int lambda_init_captured_empty = lambda_init_capture_pack_values();
  int lambda_init_captured_pack = lambda_init_capture_pack_values(1, 2, 3);
  int lambda_init_captured_call =
      lambda_init_capture_pack_call_values(1, 2L, 'c');
  int lambda_init_captured_template_call =
      lambda_init_capture_pack_template_call_values(1, 2L, 'c');
  int lambda_init_captured_size =
      lambda_init_capture_pack_size_values(1, 2L, 'c');
  (void)empty_types;
  (void)type_pack;
  (void)empty_ints;
  (void)int_pack;
  (void)alias_tuple;
  (void)wrapped;
  (void)split;
  (void)boxed;
  (void)decltype_tuple;
  (void)value;
  (void)count;
  (void)value_count;
  (void)alias_count;
  (void)forwarded;
  (void)forwarded_incremented;
  (void)forwarded_mapped;
  (void)forwarded_const_ref;
  (void)forwarded_rvalue_ref;
  (void)forwarded_cast;
  (void)boxed_parameters;
  (void)decltype_parameters;
  (void)decltype_forward_parameters;
  (void)direct_constructed;
  (void)direct_constructed_incremented;
  (void)direct_constructed_mapped;
  (void)new_constructed;
  (void)new_constructed_mapped;
  (void)temporary_constructed;
  (void)temporary_constructed_incremented;
  (void)braced_temporary_constructed;
  (void)braced_temporary_constructed_incremented;
  (void)member_initialized;
  (void)expanded_bases;
  (void)member_using;
  (void)member_using_type;
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
  (void)mapped_fold;
  (void)incremented_difference;
  (void)seeded_mapped_fold;
  (void)seeded_incremented_fold;
  (void)braced;
  (void)braced_incremented;
  (void)braced_mapped;
  (void)lambda_captured;
  (void)lambda_captured_empty;
  (void)lambda_captured_ref;
  (void)lambda_captured_call;
  (void)lambda_captured_template_call;
  (void)lambda_captured_size;
  (void)lambda_init_captured;
  (void)lambda_init_captured_empty;
  (void)lambda_init_captured_pack;
  (void)lambda_init_captured_call;
  (void)lambda_init_captured_template_call;
  (void)lambda_init_captured_size;
}
