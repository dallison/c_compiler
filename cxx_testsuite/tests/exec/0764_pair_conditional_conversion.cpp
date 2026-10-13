// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>
#include <utility>

using NullPair = std::pair<std::nullptr_t, int>;
using PointerPair = std::pair<const void*, int>;

static_assert(std::is_constructible_v<PointerPair, NullPair&&>);
static_assert(!std::is_constructible_v<NullPair, PointerPair&&>);

PointerPair choose(bool first) {
  return first ? std::make_pair(nullptr, 7)
               : PointerPair{nullptr, 9};
}

int main() {
  PointerPair first = choose(true);
  PointerPair second = choose(false);
  return first.first == nullptr && first.second == 7 &&
                 second.first == nullptr && second.second == 9
             ? 0
             : 1;
}
