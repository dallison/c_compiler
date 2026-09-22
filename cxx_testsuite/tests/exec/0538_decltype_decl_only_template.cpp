// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class Key>
static auto GetReturnType(int) -> decltype(sizeof(Key));
template <class Key>
static int GetReturnType(...);

int main() {
  using ReturnType = decltype(GetReturnType<int>(0));
  return std::is_same<ReturnType, decltype(sizeof(int))>::value ? 0 : 1;
}
