// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class... Values>
bool fold_equal(Values... values) {
  return (... == values);
}

template <class... Values>
bool fold_not_equal(Values... values) {
  return (... != values);
}

template <class... Values>
bool fold_less(Values... values) {
  return (... < values);
}

template <class... Values>
bool fold_greater(Values... values) {
  return (... > values);
}

template <class... Values>
bool fold_less_equal(Values... values) {
  return (... <= values);
}

template <class... Values>
bool fold_greater_equal(Values... values) {
  return (... >= values);
}

template <class... Values>
int fold_assign(int initial, Values... values) {
  int result = initial;
  (result = ... = values);
  return result;
}

template <class... Values>
int fold_plus_assign(int initial, Values... values) {
  int result = initial;
  (result += ... += values);
  return result;
}

template <class... Values>
int fold_minus_assign(int initial, Values... values) {
  int result = initial;
  (result -= ... -= values);
  return result;
}

template <class... Values>
int fold_multiply_assign(int initial, Values... values) {
  int result = initial;
  (result *= ... *= values);
  return result;
}

template <class... Values>
int fold_divide_assign(int initial, Values... values) {
  int result = initial;
  (result /= ... /= values);
  return result;
}

template <class... Values>
int fold_remainder_assign(int initial, Values... values) {
  int result = initial;
  (result %= ... %= values);
  return result;
}

template <class... Values>
int fold_left_shift_assign(int initial, Values... values) {
  int result = initial;
  (result <<= ... <<= values);
  return result;
}

template <class... Values>
int fold_right_shift_assign(int initial, Values... values) {
  int result = initial;
  (result >>= ... >>= values);
  return result;
}

template <class... Values>
int fold_and_assign(int initial, Values... values) {
  int result = initial;
  (result &= ... &= values);
  return result;
}

template <class... Values>
int fold_or_assign(int initial, Values... values) {
  int result = initial;
  (result |= ... |= values);
  return result;
}

template <class... Values>
int fold_xor_assign(int initial, Values... values) {
  int result = initial;
  (result ^= ... ^= values);
  return result;
}

template <class... Values>
void fold_plus_assign_right(int seed, Values&... values) {
  (values += ... += seed);
}

template <class... Values>
int comma_last(Values... values) {
  return (0, ..., values);
}

static int comma_visits;

template <class... Values>
void visit_with_comma(Values... values) {
  (..., (++comma_visits, values));
}

struct Leaf {
  int value;
};

struct Middle {
  Leaf leaf;
  Leaf* leaf_pointer;
};

struct Root {
  Middle middle;
  Middle* middle_pointer;
};

template <class... Members>
int fold_dot_star(Root& root, Members... members) {
  return (root .* ... .* members);
}

template <class... Members>
int fold_arrow_star(Root* root, Members... members) {
  return (root ->* ... ->* members);
}

int main() {
  if (!fold_equal(7, 7) || fold_equal(7, 8)) return 1;
  if (!fold_not_equal(7, 8) || fold_not_equal(7, 7)) return 2;
  if (!fold_less(3, 4) || fold_less(4, 3)) return 3;
  if (!fold_greater(4, 3) || fold_greater(3, 4)) return 4;
  if (!fold_less_equal(4, 4) || fold_less_equal(5, 4)) return 5;
  if (!fold_greater_equal(4, 4) || fold_greater_equal(3, 4)) return 6;

  if (fold_assign(9, 4, 2) != 2) return 7;
  if (fold_plus_assign(1, 2, 3) != 6) return 8;
  if (fold_minus_assign(20, 3, 4) != 13) return 9;
  if (fold_multiply_assign(2, 3, 4) != 24) return 10;
  if (fold_divide_assign(100, 2, 5) != 10) return 11;
  if (fold_remainder_assign(29, 20, 6) != 3) return 12;
  if (fold_left_shift_assign(1, 2, 1) != 8) return 13;
  if (fold_right_shift_assign(64, 2, 1) != 8) return 14;
  if (fold_and_assign(15, 14, 11) != 10) return 15;
  if (fold_or_assign(1, 2, 4) != 7) return 16;
  if (fold_xor_assign(15, 3, 5) != 9) return 17;

  int first = 1;
  int second = 2;
  int third = 3;
  fold_plus_assign_right(4, first, second, third);
  if (first != 10 || second != 9 || third != 7) return 18;

  if (comma_last(1, 2, 9) != 9 || comma_last() != 0) return 19;
  visit_with_comma(1, 2, 3);
  visit_with_comma();
  if (comma_visits != 3) return 20;

  Leaf leaf{42};
  Middle middle{Leaf{17}, &leaf};
  Root root{Middle{Leaf{11}, &leaf}, &middle};
  if (fold_dot_star(root, &Root::middle, &Middle::leaf, &Leaf::value) != 11)
    return 21;
  if (fold_arrow_star(&root, &Root::middle_pointer, &Middle::leaf_pointer,
                      &Leaf::value) != 42)
    return 22;

  return 0;
}
