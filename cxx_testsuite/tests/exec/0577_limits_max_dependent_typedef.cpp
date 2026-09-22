// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <limits>
#include <memory>
#include <cstddef>

template <typename T, typename A = std::allocator<T>>
class FixedArray {
  using AllocatorTraits = std::allocator_traits<A>;
  using difference_type = typename AllocatorTraits::difference_type;
  using value_type = T;

 public:
  constexpr long max_size() const {
    return (std::numeric_limits<difference_type>::max)() / sizeof(value_type);
  }
};

int main() {
  FixedArray<int> a;
  return a.max_size() > 0 ? 0 : 1;
}
