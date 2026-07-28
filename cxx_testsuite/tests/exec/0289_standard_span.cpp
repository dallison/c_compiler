// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <array>
#include <span>
#include <type_traits>
#include <vector>

static_assert(std::dynamic_extent == static_cast<std::size_t>(-1));
static_assert(std::span<int>::extent == std::dynamic_extent);
static_assert(std::span<int, 4>::extent == 4);
static_assert(std::is_same_v<std::span<int, 3>::value_type, int>);
static_assert(std::is_same_v<std::span<const int, 3>::element_type, const int>);
static_assert(
    std::is_constructible_v<std::span<const int>, const std::array<int, 3>&>);

int main() {
  int values[] = {1, 2, 3, 4, 5};
  std::span all(values);
  using all_type = decltype(all);
  static_assert(all_type::extent == 5);
  if (all.size() != 5 || all.size_bytes() != 5 * sizeof(int) ||
      all.front() != 1 || all.back() != 5) {
    return 1;
  }

  all[2] = 8;
  if (values[2] != 8) {
    return 2;
  }

  auto first = all.first<2>();
  auto last = all.last(2);
  auto middle = all.subspan<1, 3>();
  auto tail = all.subspan(2);
  using first_type = decltype(first);
  using middle_type = decltype(middle);
  static_assert(first_type::extent == 2);
  static_assert(middle_type::extent == 3);
  if (first[1] != 2 || last[0] != 4 || middle[1] != 8 ||
      tail.size() != 3 || tail[2] != 5) {
    return 3;
  }

  std::array<int, 3> array_values{7, 9, 11};
  std::span<int, 3> array_span(array_values);
  std::span<const int> const_span = array_span;
  if (const_span.size() != 3 || const_span[1] != 9) {
    return 4;
  }

  int sum = 0;
  for (int value : const_span) {
    sum += value;
  }
  if (sum != 27 || *const_span.rbegin() != 11) {
    return 5;
  }

  auto bytes = std::as_bytes(array_span);
  auto writable = std::as_writable_bytes(array_span);
  if (bytes.size() != sizeof(array_values) ||
      writable.size() != sizeof(array_values)) {
    return 6;
  }

  std::byte bits = static_cast<std::byte>(3);
  bits <<= 2;
  bits |= static_cast<std::byte>(1);
  if (std::to_integer<unsigned int>(bits) != 13) {
    return 7;
  }

  std::span<int> empty;
  if (!empty.empty() || empty.begin() != empty.end()) {
    return 8;
  }

  std::vector<int> vector_values{4, 6, 10};
  std::span<int> vector_span(vector_values);
  vector_span[1] = 7;
  return vector_values[1] == 7 && vector_span.size() == 3 ? 0 : 9;
}
