// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <typename A>
struct allocator_is_nothrow : std::false_type {};

template <typename T, typename A>
struct Vec {
  using allocator_type = A;
  using value_type = T;
  Vec() noexcept(allocator_is_nothrow<allocator_type>::value ||
                 std::is_nothrow_move_constructible<value_type>::value) {}
};

int main() {
  Vec<int, int> v;
  return 0;
}
