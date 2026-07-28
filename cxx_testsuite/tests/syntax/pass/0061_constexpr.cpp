// RUN: -target x86_64 -std=c++20
constexpr int add(int left, int right) {
  return left + right;
}

constexpr int with_local(int value) {
  constexpr int bias = 3;
  return value + bias;
}

constexpr bool use_explicit(void) {
  return add(1, 1) == 2;
}

constexpr int branch(bool use_left) {
  if (use_left) {
    return 11;
  }
  return 22;
}

constexpr int nested_block(void) {
  {
    constexpr int inner = 6;
    return inner + 1;
  }
}

constexpr int while_false(void) {
  while (false) {
    return 0;
  }
  return 12;
}

constexpr int do_return(void) {
  do {
    return 13;
  } while (true);
}

constexpr int for_false(void) {
  for (; false;) {
    return 0;
  }
  return 14;
}

constexpr int switch_value(int value) {
  switch (value) {
    case 1:
      return 15;
    case 2:
      return 16;
    default:
      return 17;
  }
}

constexpr int mutate_local(int value) {
  int result = value;
  result += 3;
  result *= 2;
  return result;
}

constexpr int prefix_postfix(void) {
  int value = 1;
  int first = value++;
  int second = ++value;
  return first * 10 + second;
}

constexpr int comma_sequence(void) {
  int value = 1;
  return (value += 2, value *= 3, value);
}

constexpr int sum_to(int limit) {
  int sum = 0;
  for (int i = 0; i <= limit; ++i) {
    sum += i;
  }
  return sum;
}

constexpr int count_down(int value) {
  int count = 0;
  while (value > 0) {
    --value;
    ++count;
  }
  return count;
}

struct Pair {
  int left;
  int right;

  constexpr int sum(void) const {
    return left + right;
  }

  constexpr int add_to_left(int value) {
    left += value;
    return left;
  }
};

constexpr int read_pair_member(int Pair::*member, Pair value) {
  return value.*member;
}

struct Math {
  static constexpr int triple(int value) {
    return value * 3;
  }
  constexpr static int quadruple(int value) {
    return value * 4;
  }
};

struct Point {
  int x;
  int y;

  constexpr Point(int x_value, int y_value) : x(x_value), y(y_value) {}

  constexpr int sum(void) const {
    return x + y;
  }
};

struct NestedObject {
  Pair pair;
  int values[2];

  constexpr int total(void) const {
    return pair.sum() + values[0] + values[1];
  }
};

struct MixedNestedObject {
  Pair pair;
  Point point;
  int values[2];

  constexpr int total(void) const {
    return pair.sum() + point.sum() + values[0] + values[1];
  }
};

constexpr int aggregate_values(void) {
  int values[3] = {1, 2, 3};
  Pair pair = {4, 5};
  return values[1] + pair.sum();
}

constexpr int mutate_aggregate(void) {
  int values[2] = {1, 2};
  Pair pair = {3, 4};
  values[0] += 5;
  ++pair.right;
  pair.add_to_left(values[0] - pair.left);
  return pair.left + pair.right + values[1];
}

constexpr int constructed_object(void) {
  Point point(2, 3);
  return point.sum();
}

constexpr Point global_point(7, 8);
constexpr NestedObject global_nested = {{2, 3}, {4, 5}};
constexpr Point global_points[2] = {Point(1, 2), Point(3, 4)};
constexpr MixedNestedObject global_mixed[2] = {
    {{1, 2}, Point(3, 4), {5, 6}},
    {{7, 8}, Point(9, 10), {11, 12}},
};
constexpr MixedNestedObject global_mixed_copy = global_mixed[1];

constexpr int nested_object_values(void) {
  NestedObject nested = {{6, 7}, {8, 9}};
  return nested.total() + nested.pair.left + nested.values[1];
}

constexpr int mutate_nested_object(void) {
  NestedObject nested = {{1, 2}, {3, 4}};
  nested.pair.left += nested.values[0];
  nested.values[1] += nested.pair.right;
  return nested.total();
}

constexpr int class_array_values(void) {
  Point points[2] = {Point(5, 6), Point(7, 8)};
  return points[0].sum() + points[1].x + points[1].y;
}

constexpr int mutate_class_array(void) {
  Point points[2] = {Point(1, 2), Point(3, 4)};
  points[0].x += points[1].y;
  points[1].y += points[0].sum();
  return points[0].sum() + points[1].sum();
}

constexpr int mixed_nested_array_values(void) {
  MixedNestedObject items[2] = {
      {{1, 2}, Point(3, 4), {5, 6}},
      {{7, 8}, Point(9, 10), {11, 12}},
  };
  return items[0].total() + items[1].pair.sum() + items[1].point.x +
         items[0].values[1];
}

constexpr int mutate_mixed_nested_array(void) {
  MixedNestedObject items[2] = {
      {{1, 2}, Point(3, 4), {5, 6}},
      {{7, 8}, Point(9, 10), {11, 12}},
  };
  items[0].values[1] += items[1].pair.left;
  items[1].point.x += items[0].pair.sum();
  items[0].point.y += items[1].values[0];
  return items[0].total() + items[1].total();
}

constexpr int copy_mixed_nested_object(void) {
  MixedNestedObject original = {{1, 2}, Point(3, 4), {5, 6}};
  MixedNestedObject copy = original;
  copy.pair.left += 10;
  copy.point.x += original.values[0];
  copy.values[1] += original.point.y;
  return original.total() + copy.total();
}

constexpr int assign_mixed_nested_object(void) {
  MixedNestedObject items[2] = {
      {{1, 2}, Point(3, 4), {5, 6}},
      {{7, 8}, Point(9, 10), {11, 12}},
  };
  items[1] = items[0];
  items[0].pair.left = 20;
  items[1].point.y += 30;
  return items[0].total() + items[1].total();
}

constexpr MixedNestedObject make_mixed_nested_object(int bias) {
  MixedNestedObject result = {{1 + bias, 2 + bias}, Point(3 + bias, 4 + bias),
                              {5 + bias, 6 + bias}};
  return result;
}

constexpr MixedNestedObject return_mixed_nested_copy(void) {
  MixedNestedObject original = {{1, 2}, Point(3, 4), {5, 6}};
  return original;
}

constexpr int returned_mixed_nested_object_values(void) {
  MixedNestedObject item = make_mixed_nested_object(2);
  MixedNestedObject copy = return_mixed_nested_copy();
  copy.point.y += item.pair.left;
  return item.total() + copy.total();
}

constexpr MixedNestedObject adjust_mixed_nested_object(MixedNestedObject value,
                                                       int delta) {
  value.pair.left += delta;
  value.point.y += value.values[0];
  return value;
}

constexpr int take_mixed_nested_object(MixedNestedObject value) {
  value.values[0] += 1;
  return value.total();
}

constexpr int by_value_mixed_nested_object_values(void) {
  MixedNestedObject original = {{1, 2}, Point(3, 4), {5, 6}};
  MixedNestedObject adjusted = adjust_mixed_nested_object(original, 10);
  original.point.x = 30;
  return original.total() + adjusted.total() +
         take_mixed_nested_object(make_mixed_nested_object(1));
}

constexpr int pointer_reference_values(void) {
  int values[2] = {4, 5};
  int* ptr = &values[0];
  int& ref = values[1];
  ref += *ptr;
  *ptr += 2;
  return values[0] + values[1];
}

struct ScopedDestructible {
  int value;
  constexpr ScopedDestructible(int initial) : value(initial) {}
  constexpr ~ScopedDestructible() { value = value + 1; }
};

constexpr int scoped_destructor_value(void) {
  {
    ScopedDestructible item(8);
  }
  return 19;
}

constexpr MixedNestedObject global_mixed_from_factory =
    make_mixed_nested_object(3);

constexpr int lambda_value(void) {
  auto fn = [](int value) constexpr {
    return value + 6;
  };
  return fn(7);
}

consteval int immediate_value(int value) {
  return value + 8;
}

struct ConstinitMembers {
  inline static constinit int value = immediate_value(5);
};

consteval MixedNestedObject immediate_mixed_nested_object(int bias) {
  return make_mixed_nested_object(bias);
}

template <typename T>
constexpr T templated_add(T left, T right) {
  return left + right;
}

template <int N>
constexpr int nttp_value(void) {
  return N;
}

constexpr int if_constexpr_value(void) {
  if constexpr (true) {
    return 21;
  } else {
    return 0;
  }
}

constexpr int if_constexpr_discarded_branch(void) {
  if constexpr (false) {
    int* invalid = 0;
    return invalid;
  } else {
    return 31;
  }
}

constexpr int base = 4;
constexpr int total = add(base, 5);
constinit int initialized = immediate_value(2);
thread_local constinit int thread_initialized = immediate_value(3);
constexpr MixedNestedObject global_immediate_mixed =
    immediate_mixed_nested_object(1);

template <int N>
constexpr int sum_const_array(const int (&values)[N]) {
  int result = 0;
  for (int i = 0; i < N; ++i) {
    result += values[i];
  }
  return result;
}

constexpr int const_array_values[3] = {1, 2, 3};

static_assert(add(2, 3) == 5, "constexpr function call");
static_assert(with_local(4) == 7, "constexpr local declaration");
static_assert(base == 4, "constexpr variable");
static_assert(branch(true) == 11, "constexpr if true");
static_assert(branch(false) == 22, "constexpr if false");
static_assert(nested_block() == 7, "constexpr nested block");
static_assert(while_false() == 12, "constexpr while");
static_assert(do_return() == 13, "constexpr do");
static_assert(for_false() == 14, "constexpr for");
static_assert(switch_value(2) == 16, "constexpr switch case");
static_assert(switch_value(9) == 17, "constexpr switch default");
static_assert(mutate_local(4) == 14, "constexpr mutation");
static_assert(prefix_postfix() == 13, "constexpr increments");
static_assert(comma_sequence() == 9, "constexpr comma");
static_assert(sum_to(5) == 15, "constexpr for mutation");
static_assert(count_down(4) == 4, "constexpr while mutation");
static_assert(aggregate_values() == 11, "constexpr aggregate objects");
static_assert(read_pair_member(&Pair::right, Pair{4, 7}) == 7,
              "constexpr member-pointer and aggregate arguments");
static_assert(mutate_aggregate() == 13, "constexpr aggregate mutation");
static_assert(constructed_object() == 5, "constexpr constructor");
static_assert(global_point.sum() == 15, "global constexpr constructor");
static_assert(nested_object_values() == 45, "constexpr nested objects");
static_assert(mutate_nested_object() == 15, "constexpr nested mutation");
static_assert(global_nested.total() == 14, "global nested constexpr object");
static_assert(class_array_values() == 26, "constexpr class arrays");
static_assert(mutate_class_array() == 21, "constexpr class array mutation");
static_assert(global_points[0].sum() + global_points[1].sum() == 10,
              "global constexpr class array");
static_assert(mixed_nested_array_values() == 51,
              "constexpr mixed nested arrays");
static_assert(mutate_mixed_nested_array() == 99,
              "constexpr mixed nested array mutation");
static_assert(global_mixed[0].total() + global_mixed[1].point.sum() == 40,
              "global constexpr mixed nested array");
static_assert(copy_mixed_nested_object() == 61,
              "constexpr mixed nested object copy");
static_assert(assign_mixed_nested_object() == 91,
              "constexpr mixed nested object assignment");
static_assert(global_mixed_copy.total() == 57,
              "global constexpr mixed nested object copy");
static_assert(returned_mixed_nested_object_values() == 57,
              "constexpr mixed nested object return");
static_assert(global_mixed_from_factory.total() == 39,
              "global constexpr mixed nested object factory");
static_assert(global_immediate_mixed.total() == 27,
              "consteval mixed nested object");
static_assert(by_value_mixed_nested_object_values() == 112,
              "constexpr mixed nested object by-value parameter");
static_assert(pointer_reference_values() == 15,
              "constexpr pointer and reference values");
static_assert(sum_const_array(const_array_values) == 6,
              "constexpr const array reference deduction");
static_assert(scoped_destructor_value() == 19,
              "constexpr scoped destructor");
static_assert(Math::triple(4) == 12, "constexpr static member");
static_assert(Math::quadruple(4) == 16, "static constexpr member");
static_assert(lambda_value() == 13, "constexpr lambda");
static_assert(immediate_value(3) == 11, "consteval call");
static_assert(if_constexpr_value() == 21, "if constexpr");
static_assert(if_constexpr_discarded_branch() == 31,
              "if constexpr discarded branch");
static_assert(templated_add<int>(4, 6) == 10, "constexpr function template");
static_assert(nttp_value<immediate_value(2)>() == 10, "constexpr NTTP");

struct ExplicitByConstant {
  int value;
  explicit(use_explicit()) operator int() const {
    return value;
  }
  explicit(false) operator bool() const {
    return value != 0;
  }
};

int main(void) {
  static constinit int local_static_initialized = immediate_value(4);
  ExplicitByConstant value;
  value.value = 1;
  int as_int = static_cast<int>(value);
  bool as_bool = value;
  auto lambda = [](int value) constexpr {
    return value + 1;
  };
  return as_int + as_bool + lambda(1) + thread_initialized +
         local_static_initialized + ConstinitMembers::value;
}
