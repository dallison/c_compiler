// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <array>
#include <stdexcept>
#include <type_traits>

static_assert(std::tuple_size<std::array<int, 3> >::value == 3,
              "array tuple_size");
static_assert(std::is_same<std::tuple_element_t<1, std::array<int, 3> >,
                           int>::value,
              "array tuple_element");

int iterator_api(void) {
  std::array<int, 4> values = {{1, 2, 3, 4}};
  int sum = 0;
  for (int* it = values.begin(); it != values.end(); ++it) {
    sum += *it;
  }
  const std::array<int, 4>& cvalues = values;
  sum += *(cvalues.cbegin() + 1);
  sum += *values.rbegin();
  sum += *values.crbegin();
  return sum;
}

int element_access(void) {
  std::array<int, 3> values = {{5, 6, 7}};
  values[1] = 10;
  values.at(2) = 20;
  return values.front() + values.back() + values.data()[1];
}

int at_throws(void) {
  std::array<int, 2> values = {{1, 2}};
  try {
    values.at(2);
  } catch (const std::out_of_range&) {
    return 1;
  }
  return 0;
}

int fill_swap_compare(void) {
  std::array<int, 3> left = {{1, 2, 3}};
  std::array<int, 3> right = {{4, 5, 6}};
  left.fill(9);
  left.swap(right);
  std::array<int, 3> expected_left = {{4, 5, 6}};
  std::array<int, 3> expected_right = {{9, 9, 9}};
  std::array<int, 2> low = {{1, 2}};
  std::array<int, 2> high = {{1, 3}};
  std::array<int, 2> greater = {{2, 0}};
  std::array<int, 2> lesser = {{1, 9}};
  if (!(left == expected_left)) {
    return 1;
  }
  if (!(right == expected_right)) {
    return 2;
  }
  if (!(low < high)) {
    return 3;
  }
  if (!(greater > lesser)) {
    return 4;
  }
  return 0;
}

int tuple_like_api(void) {
  std::array<int, 3> values = {{11, 12, 13}};
  std::get<1>(values) = 20;
  auto [a, b, c] = values;
  return a + b + c;
}

int to_array_api(void) {
  int raw[3] = {14, 15, 16};
  auto values = std::to_array(raw);
  return values[0] + values[1] + values[2];
}

int zero_size_api(void) {
  std::array<int, 0> values = {};
  if (!values.empty() || values.size() != 0 || values.max_size() != 0) {
    return 1;
  }
  if (values.begin() != values.end()) {
    return 2;
  }
  try {
    values.at(0);
  } catch (const std::out_of_range&) {
    return 0;
  }
  return 3;
}

int main(void) {
  if (iterator_api() != 20) {
    return 1;
  }
  if (element_access() != 35) {
    return 2;
  }
  if (!at_throws()) {
    return 3;
  }
  if (fill_swap_compare() != 0) {
    return 4;
  }
  if (tuple_like_api() != 44) {
    return 5;
  }
  if (to_array_api() != 45) {
    return 6;
  }
  if (zero_size_api() != 0) {
    return 7;
  }
  return 0;
}
