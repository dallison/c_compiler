#include <cstddef>
#include <span>

template <class IndexType, std::size_t... Values>
struct shape {};

using value = shape<int, 2, 3, 4>;
using dynamic_value = shape<int, 2, std::dynamic_extent, 4>;

#include <array>

template <class IndexType, std::size_t... Values>
struct nested_shape {
  static constexpr std::size_t count() {
    return ((Values == 3 ? 1 : 0) + ... + 0);
  }

  std::array<IndexType, count()> storage;
};

using nested_value = nested_shape<int, 2, 3, 4>;
nested_value object;
