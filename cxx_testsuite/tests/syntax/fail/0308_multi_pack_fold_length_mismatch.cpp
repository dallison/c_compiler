// RUN: -std=c++20
// EXPECT: pack expansion argument packs have different lengths

template <class... Types>
struct type_pack {};

template <class Left, class Right>
struct mismatched_fold;

template <class... Left, class... Right>
struct mismatched_fold<type_pack<Left...>, type_pack<Right...>> {
  static int sum(Left... left, Right... right) {
    return (... + (left + right));
  }
};

int use_mismatched_fold() {
  return mismatched_fold<type_pack<int, int>,
                         type_pack<long>>::sum(1, 2, 3L);
}
