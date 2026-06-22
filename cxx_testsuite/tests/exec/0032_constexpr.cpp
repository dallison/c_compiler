// RUN: -std=c++20
constexpr int add(int left, int right) {
  return left + right;
}

constexpr int with_local(int value) {
  constexpr int bias = 3;
  return value + bias;
}

constexpr int choose(bool use_left, int left, int right) {
  return use_left ? left : right;
}

constexpr bool force_explicit(void) {
  return true;
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
constexpr MixedNestedObject global_immediate_mixed =
    immediate_mixed_nested_object(1);
constexpr int control_total =
    branch(true) + branch(false) + nested_block() + while_false() +
    do_return() + for_false() + switch_value(2) + switch_value(9);
constexpr int mutation_total =
    mutate_local(4) + prefix_postfix() + comma_sequence() + sum_to(5) +
    count_down(4);
constexpr int object_total = aggregate_values();
constexpr int object_mutation_total = mutate_aggregate();
constexpr int constructed_total = constructed_object();
constexpr int global_constructed_total = global_point.sum();
constexpr int nested_object_total = nested_object_values();
constexpr int nested_mutation_total = mutate_nested_object();
constexpr int global_nested_total = global_nested.total();
constexpr int class_array_total = class_array_values();
constexpr int class_array_mutation_total = mutate_class_array();
constexpr int global_class_array_total =
    global_points[0].sum() + global_points[1].sum();
constexpr int mixed_nested_array_total = mixed_nested_array_values();
constexpr int mixed_nested_array_mutation_total = mutate_mixed_nested_array();
constexpr int global_mixed_nested_array_total =
    global_mixed[0].total() + global_mixed[1].point.sum();
constexpr int mixed_nested_copy_total = copy_mixed_nested_object();
constexpr int mixed_nested_assignment_total = assign_mixed_nested_object();
constexpr int global_mixed_copy_total = global_mixed_copy.total();
constexpr int returned_mixed_nested_object_total =
    returned_mixed_nested_object_values();
constexpr int global_mixed_from_factory_total =
    global_mixed_from_factory.total();
constexpr int by_value_mixed_nested_object_total =
    by_value_mixed_nested_object_values();
constexpr int global_immediate_mixed_total = global_immediate_mixed.total();
constexpr int pointer_reference_total = pointer_reference_values();
constexpr int static_member_total = Math::triple(4);
constexpr int static_member_alt_total = Math::quadruple(4);
constexpr int lambda_total = lambda_value();
constexpr int immediate_total = immediate_value(3) + if_constexpr_value() +
                                if_constexpr_discarded_branch();
constexpr int template_total = templated_add<int>(4, 6);
constexpr int nttp_total = nttp_value<immediate_value(2)>();

struct ExplicitByConstant {
  int value;
  explicit(force_explicit()) operator int() const {
    return value;
  }
  explicit(false) operator bool() const {
    return value != 0;
  }
};

int main(void) {
  if (total != 9) {
    return 1;
  }
  if (with_local(4) != 7) {
    return 2;
  }
  if (choose(false, 1, 8) != 8) {
    return 3;
  }
  if (control_total != 112) {
    return 4;
  }
  if (mutation_total != 55) {
    return 5;
  }
  if (object_total != 11) {
    return 6;
  }
  if (object_mutation_total != 13) {
    return 7;
  }
  if (constructed_total != 5) {
    return 8;
  }
  if (global_constructed_total != 15) {
    return 9;
  }
  if (nested_object_total != 45) {
    return 16;
  }
  if (nested_mutation_total != 15) {
    return 17;
  }
  if (global_nested_total != 14) {
    return 18;
  }
  if (class_array_total != 26) {
    return 19;
  }
  if (class_array_mutation_total != 21) {
    return 20;
  }
  if (global_class_array_total != 10) {
    return 21;
  }
  if (mixed_nested_array_total != 51) {
    return 22;
  }
  if (mixed_nested_array_mutation_total != 99) {
    return 23;
  }
  if (global_mixed_nested_array_total != 40) {
    return 24;
  }
  if (mixed_nested_copy_total != 61) {
    return 25;
  }
  if (mixed_nested_assignment_total != 91) {
    return 26;
  }
  if (global_mixed_copy_total != 57) {
    return 27;
  }
  if (returned_mixed_nested_object_total != 57) {
    return 28;
  }
  if (global_mixed_from_factory_total != 39) {
    return 29;
  }
  if (by_value_mixed_nested_object_total != 112) {
    return 30;
  }
  if (global_immediate_mixed_total != 27) {
    return 31;
  }
  if (pointer_reference_total != 15) {
    return 32;
  }
  if (static_member_total != 12) {
    return 10;
  }
  if (static_member_alt_total != 16) {
    return 11;
  }
  if (lambda_total != 13) {
    return 12;
  }
  if (immediate_total != 63) {
    return 13;
  }
  if (initialized != 10) {
    return 14;
  }
  if (template_total != 10) {
    return 15;
  }
  if (nttp_total != 10) {
    return 16;
  }
  initialized = 12;
  if (initialized != 12) {
    return 17;
  }
  ExplicitByConstant value;
  value.value = 6;
  int as_int = static_cast<int>(value);
  if (as_int != 6) {
    return 18;
  }
  bool as_bool = value;
  if (!as_bool) {
    return 19;
  }
  return 0;
}
