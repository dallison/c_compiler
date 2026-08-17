// RUN: -std=c++20
// EXPECT_EXIT: 0

template <typename... Left, typename... Right>
int deduced_lengths(Left... left, Right... right) {
  (void)sizeof...(left);
  (void)sizeof...(right);
  return sizeof...(Left) == 0 && sizeof...(Right) == 4;
}

template <typename... Left, typename... Right>
int paired_sum(Left... left, Right... right) {
  return (0 + ... + (left + right));
}

template <typename... Left, typename... Right>
int paired_array_sum(Left... left, Right... right) {
  int values[] = {(left + right)...};
  return values[0] + values[1];
}

int main() {
  // A non-trailing function parameter pack is a non-deduced context. All four
  // arguments therefore belong to Right.
  if (!deduced_lengths(1, 2, 3, 4)) {
    return 1;
  }

  // Explicit arguments bind Left; Right is then deduced from the remaining
  // call arguments. The fold expands both packs in lockstep.
  if (paired_sum<int, int>(1, 2, 10, 20) != 33) {
    return 2;
  }
  if (paired_array_sum<int, int>(3, 4, 30, 40) != 77) {
    return 3;
  }
  return 0;
}
