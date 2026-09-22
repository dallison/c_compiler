// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <limits>
#include <cstddef>

template <class T>
struct Fixed {
  using difference_type = long;
  using value_type = T;
  constexpr long max_size() const {
    return (std::numeric_limits<difference_type>::max)() / sizeof(value_type);
  }
  bool empty() const { return false; }
};

int main() {
  Fixed<int> f;
  return f.max_size() > 0 && !f.empty() ? 0 : 1;
}
