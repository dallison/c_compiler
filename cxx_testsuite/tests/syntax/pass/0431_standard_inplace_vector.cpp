// RUN: -std=c++26

#include <array>
#include <inplace_vector>
#include <type_traits>
#include <version>

#if __cpp_lib_inplace_vector != 202603L
#error "__cpp_lib_inplace_vector has the wrong value"
#endif

#if __cpp_lib_constexpr_inplace_vector != 202502L
#error "__cpp_lib_constexpr_inplace_vector has the wrong value"
#endif

#if __cpp_lib_optional != 202506L
#error "__cpp_lib_optional has the wrong value"
#endif

using vector_type = std::inplace_vector<int, 8>;

static_assert(std::is_same_v<vector_type::value_type, int>);
static_assert(std::is_same_v<vector_type::iterator, int*>);
static_assert(std::is_same_v<vector_type::const_iterator, const int*>);
static_assert(std::is_same_v<
              decltype(std::declval<vector_type&>().push_back(
                  std::declval<int&&>())),
              int&>);
static_assert(std::is_same_v<
              decltype(std::declval<std::inplace_vector<int, 0>&>().push_back(
                  std::declval<int&&>())),
              int&>);
static_assert(vector_type::capacity() == 8);
static_assert(vector_type::max_size() == 8);
static_assert(std::is_trivially_copyable_v<std::inplace_vector<int, 0>>,
              "zero-capacity specialization must be trivially copyable");
static_assert(
    std::is_trivially_default_constructible_v<std::inplace_vector<int, 0>>,
    "zero-capacity specialization must be trivially default constructible");

struct less_only {
  int value;
  bool operator<(const less_only& other) const {
    return value < other.value;
  }
};

auto compare_less_only(const std::inplace_vector<less_only, 2>& left,
                       const std::inplace_vector<less_only, 2>& right) {
  return left <=> right;
}

void exercise_api() {
  int source[] = {1, 2, 3};
  vector_type values(source, source + 3);
  std::array<int, 3> source_range{1, 2, 3};
  vector_type ranged(std::from_range, source_range);
  vector_type copied(values);
  vector_type moved(static_cast<vector_type&&>(copied));

  values.assign(2, 4);
  values.assign(source + 0, source + 3);
  values.assign({5, 6});
  values.assign_range(source_range);
  values.insert(values.begin(), 7);
  values.insert(values.begin(), 2, 8);
  values.insert(values.end(), source + 0, source + 3);
  values.insert(values.end(), std::initializer_list<int>{9});
  values.insert_range(values.begin(), source_range);
  values.emplace(values.begin(), 10);
  values.emplace_back(11);
  values.try_emplace_back(12);
  values.unchecked_emplace_back(13);
  values.push_back(14);
  values.try_push_back(15);
  values.unchecked_push_back(16);
  values.append_range(source_range);
  values.pop_back();
  values.erase(values.begin());
  values.erase(values.begin(), values.end());
  values.resize(2);
  values.resize(3, 17);
  values.reserve(8);
  values.shrink_to_fit();
  values.clear();
  values.swap(ranged);
  std::swap(values, moved);
  (void)std::erase(values, 1);
  (void)std::erase_if(values, [](int value) { return value == 2; });
}
