// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <algorithm>

#if __cpp_lib_sample != 201603L
#error "__cpp_lib_sample has the wrong value"
#endif

struct rejection_generator {
  using result_type = unsigned;

  const unsigned* values;
  unsigned index = 0;

  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 5; }
  result_type operator()() { return values[index++]; }
};

struct input_iterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = int;
  using difference_type = long;
  using pointer = const int*;
  using reference = int;

  const int* current;

  int operator*() const { return *current; }
  input_iterator& operator++() {
    ++current;
    return *this;
  }
  input_iterator operator++(int) {
    input_iterator copy = *this;
    ++current;
    return copy;
  }
  friend bool operator==(input_iterator left, input_iterator right) {
    return left.current == right.current;
  }
  friend bool operator!=(input_iterator left, input_iterator right) {
    return !(left == right);
  }
};

int main() {
  const unsigned shuffle_draws[] = {4, 1, 0, 0};
  rejection_generator shuffle_generator{shuffle_draws};
  int shuffled[] = {0, 1, 2, 3};
  std::shuffle(shuffled, shuffled + 4, shuffle_generator);
  if (shuffled[0] != 3 || shuffled[1] != 2 ||
      shuffled[2] != 0 || shuffled[3] != 1) {
    return 1;
  }

  const unsigned sample_draws[] = {4, 1, 0};
  rejection_generator sample_generator{sample_draws};
  int population[] = {10, 20, 30, 40};
  int selected = 0;
  int* selected_end =
      std::sample(population, population + 4, &selected, 1, sample_generator);
  if (selected_end != &selected + 1 || selected != 20) return 2;

  const unsigned reservoir_draws[] = {2, 4, 0};
  rejection_generator reservoir_generator{reservoir_draws};
  int reservoir[2] = {};
  int* reservoir_end =
      std::sample(input_iterator{population}, input_iterator{population + 4},
                  reservoir, 2, reservoir_generator);
  if (reservoir_end != reservoir + 2 ||
      reservoir[0] != 40 || reservoir[1] != 20) {
    return 3;
  }

  const unsigned ranges_shuffle_draws[] = {4, 1, 0, 0};
  rejection_generator ranges_shuffle_generator{ranges_shuffle_draws};
  int ranges_shuffled[] = {0, 1, 2, 3};
  auto ranges_end =
      std::ranges::shuffle(ranges_shuffled, ranges_shuffle_generator);
  if (ranges_end != ranges_shuffled + 4 ||
      ranges_shuffled[0] != 3 || ranges_shuffled[3] != 1) {
    return 4;
  }

  const unsigned ranges_sample_draws[] = {4, 1, 0};
  rejection_generator ranges_sample_generator{ranges_sample_draws};
  int ranges_selected = 0;
  int* ranges_selected_end = std::ranges::sample(
      population, &ranges_selected, 1, ranges_sample_generator);
  if (ranges_selected_end != &ranges_selected + 1 ||
      ranges_selected != 20) {
    return 5;
  }

  const unsigned all_draws[] = {0};
  rejection_generator all_generator{all_draws};
  int all[4] = {};
  int* all_end =
      std::sample(population, population + 4, all, 9, all_generator);
  if (all_end != all + 4 || all[0] != 10 || all[3] != 40) return 6;
  return 0;
}
