// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class... Types>
struct type_pack {};

template <class LeftTypes, class RightTypes>
struct function_parameter_folds;

template <class... Left, class... Right>
struct function_parameter_folds<type_pack<Left...>, type_pack<Right...>> {
  static int sum(Left... left, Right... right) {
    return (0 + ... + (left + right));
  }

  static int left_subtract(Left... left, Right... right) {
    return (... - (left + right));
  }

  static int right_subtract(Left... left, Right... right) {
    return ((left + right) - ...);
  }
};

int main() {
  using function_folds =
      function_parameter_folds<type_pack<int, int, int>,
                               type_pack<long, long, long>>;
  if (function_folds::sum(1, 2, 3, 10L, 20L, 30L) != 66) {
    return 1;
  }
  if (function_folds::left_subtract(1, 2, 3, 10L, 20L, 30L) != -44) {
    return 2;
  }
  if (function_folds::right_subtract(1, 2, 3, 10L, 20L, 30L) != 22) {
    return 3;
  }
  using empty_folds =
      function_parameter_folds<type_pack<>, type_pack<>>;
  if (empty_folds::sum() != 0) return 4;
  return 0;
}
