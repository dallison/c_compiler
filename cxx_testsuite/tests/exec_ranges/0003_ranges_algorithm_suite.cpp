// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <algorithm>
#include <ranges>
#include <vector>

struct employee {
  int id;
  int salary;
  int bonus() const { return salary / 10; }
};

// A minimal uniform random bit generator, so shuffle/sample can be exercised
// without <random>.
struct counter_generator {
  using result_type = unsigned long long;

  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 0x7fffffffull; }

  unsigned long long state = 12345;
  result_type operator()() {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    return state >> 33;
  }
};

int main() {
  int source[] = {1, 2, 3, 4, 5};

  int backward[5] = {};
  auto backward_result =
      std::ranges::copy_backward(source, backward + 5);
  if (backward_result.out != backward || backward[0] != 1 || backward[4] != 5) {
    return 1;
  }

  int moved[5] = {};
  auto moved_result = std::ranges::move_backward(source, moved + 5);
  if (moved_result.out != moved || moved[4] != 5) return 2;

  int without_three[4] = {};
  auto removed = std::ranges::remove_copy(source, without_three, 3);
  if (removed.out != without_three + 4 || without_three[2] != 4) return 3;

  int odds[3] = {};
  auto removed_if = std::ranges::remove_copy_if(
      source, odds, [](int value) { return value % 2 == 0; });
  if (removed_if.out != odds + 3 || odds[1] != 3) return 4;

  int replaced[5] = {};
  std::ranges::replace_copy(source, replaced, 3, 30);
  if (replaced[2] != 30 || replaced[0] != 1) return 5;

  int replaced_if[5] = {};
  std::ranges::replace_copy_if(
      source, replaced_if, [](int value) { return value > 3; }, 0);
  if (replaced_if[3] != 0 || replaced_if[4] != 0 || replaced_if[2] != 3) {
    return 6;
  }

  int mutable_values[] = {1, 2, 3, 4, 5};
  std::ranges::replace_if(
      mutable_values, [](int value) { return value % 2 == 0; }, 0);
  if (mutable_values[1] != 0 || mutable_values[3] != 0) return 7;

  int left[] = {1, 2, 3};
  int right[] = {4, 5, 6};
  std::ranges::swap_ranges(left, right);
  if (left[0] != 4 || right[2] != 3) return 8;

  int reversed[3] = {};
  std::ranges::reverse_copy(left, reversed);
  if (reversed[0] != 6 || reversed[2] != 4) return 9;

  int rotated[3] = {};
  std::ranges::rotate_copy(left, left + 1, rotated);
  if (rotated[0] != 5 || rotated[2] != 4) return 10;

  int duplicates[] = {1, 1, 2, 2, 2, 3};
  int deduplicated[3] = {};
  auto unique_result = std::ranges::unique_copy(duplicates, deduplicated);
  if (unique_result.out != deduplicated + 3 || deduplicated[2] != 3) return 11;

  employee staff[] = {{1, 500}, {2, 100}, {3, 400}, {4, 200}};
  auto partitioned = std::ranges::stable_partition(
      staff, [](int salary) { return salary >= 400; }, &employee::salary);
  if (partitioned.begin() != staff + 2) return 12;
  if (staff[0].id != 1 || staff[1].id != 3 || staff[2].id != 2 ||
      staff[3].id != 4) {
    return 13;
  }
  if (!std::ranges::is_partitioned(
          staff, [](int salary) { return salary >= 400; },
          &employee::salary)) {
    return 14;
  }

  int split_true[2] = {};
  int split_false[3] = {};
  auto split = std::ranges::partition_copy(
      source, split_true, split_false,
      [](int value) { return value % 2 == 0; });
  if (split.out1 != split_true + 2 || split.out2 != split_false + 3) return 15;

  int unsorted[] = {9, 4, 7, 1, 8, 3};
  int smallest[3] = {};
  auto copied = std::ranges::partial_sort_copy(unsorted, smallest);
  if (copied.out != smallest + 3) return 16;
  if (smallest[0] != 1 || smallest[1] != 3 || smallest[2] != 4) return 17;

  int sorted[] = {1, 2, 3, 4, 5, 5, 6};
  auto range = std::ranges::equal_range(sorted, 5);
  if (range.begin() != sorted + 4 || range.end() != sorted + 6) return 18;
  if (std::ranges::upper_bound(sorted, 4) != sorted + 4) return 19;
  if (std::ranges::partition_point(
          sorted, [](int value) { return value < 4; }) != sorted + 3) {
    return 20;
  }

  int merge_target[] = {1, 5, 9, 2, 3, 7};
  std::ranges::inplace_merge(merge_target, merge_target + 3);
  if (!std::ranges::is_sorted(merge_target)) return 21;

  int superset[] = {1, 2, 3, 4, 5};
  int subset[] = {2, 4};
  if (!std::ranges::includes(superset, subset)) return 22;
  int not_subset[] = {2, 7};
  if (std::ranges::includes(superset, not_subset)) return 23;

  int odd_values[] = {1, 3, 5};
  int even_values[] = {2, 3, 6};
  int union_target[6] = {};
  auto unioned =
      std::ranges::set_union(odd_values, even_values, union_target);
  if (unioned.out != union_target + 5 || union_target[4] != 6) return 24;

  int intersection_target[3] = {};
  auto intersected = std::ranges::set_intersection(odd_values, even_values,
                                                   intersection_target);
  if (intersected.out != intersection_target + 1 ||
      intersection_target[0] != 3) {
    return 25;
  }

  int difference_target[3] = {};
  auto differed =
      std::ranges::set_difference(odd_values, even_values, difference_target);
  if (differed.out != difference_target + 2 || difference_target[1] != 5) {
    return 26;
  }

  int symmetric_target[6] = {};
  auto symmetric = std::ranges::set_symmetric_difference(
      odd_values, even_values, symmetric_target);
  if (symmetric.out != symmetric_target + 4) return 27;

  std::vector<int> haystack{1, 2, 3, 4, 2, 3, 9};
  std::vector<int> needle{2, 3};
  auto found = std::ranges::search(haystack, needle);
  if (found.begin() != haystack.begin() + 1) return 28;
  auto found_last = std::ranges::find_end(haystack, needle);
  if (found_last.begin() != haystack.begin() + 4) return 29;
  std::vector<int> repeated{1, 7, 7, 7, 2};
  auto run = std::ranges::search_n(repeated, 3, 7);
  if (run.begin() != repeated.begin() + 1 || run.end() != repeated.begin() + 4) {
    return 30;
  }
  std::vector<int> any_of_these{9, 4};
  if (std::ranges::find_first_of(haystack, any_of_these) !=
      haystack.begin() + 3) {
    return 31;
  }

  if (std::ranges::min(source) != 1 || std::ranges::max(source) != 5) return 32;
  if (std::ranges::min_element(source) != source) return 33;
  if (std::ranges::max_element(source) != source + 4) return 34;
  auto extremes = std::ranges::minmax_element(source);
  if (extremes.min != source || extremes.max != source + 4) return 35;
  if (std::ranges::max(staff, std::ranges::less{}, &employee::bonus).id != 1) {
    return 36;
  }

  int permutation[] = {3, 1, 2};
  int other_permutation[] = {2, 3, 1};
  if (!std::ranges::is_permutation(permutation, other_permutation)) return 37;
  int not_permutation[] = {2, 3, 3};
  if (std::ranges::is_permutation(permutation, not_permutation)) return 38;

  std::vector<int> word1{'a', 'b', 'c'};
  std::vector<int> word2{'a', 'b', 'd'};
  if (!std::ranges::lexicographical_compare(word1, word2)) return 39;
  if (std::ranges::lexicographical_compare(word2, word1)) return 40;

  counter_generator generator;
  int deck[] = {1, 2, 3, 4, 5, 6, 7, 8};
  std::ranges::shuffle(deck, generator);
  std::ranges::sort(deck);
  for (int i = 0; i < 8; ++i) {
    if (deck[i] != i + 1) return 41;
  }

  int sampled[3] = {-1, -1, -1};
  auto sample_end = std::ranges::sample(deck, sampled, 3, generator);
  if (sample_end != sampled + 3) return 42;
  for (int value : sampled) {
    if (value < 1 || value > 8) return 43;
  }

  int heap[] = {3, 9, 1, 8, 2};
  std::ranges::make_heap(heap);
  if (!std::ranges::is_heap(heap)) return 44;
  std::ranges::pop_heap(heap);
  if (heap[4] != 9) return 45;
  return 0;
}
