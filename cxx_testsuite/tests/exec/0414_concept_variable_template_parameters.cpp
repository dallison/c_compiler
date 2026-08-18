// RUN: -std=c++26
// EXPECT_EXIT: 0

template <typename T>
concept HasSize = sizeof(T) > 0;

template <typename T>
constexpr int type_size = sizeof(T);

template <template <typename> auto Value,
          template <typename> concept Constraint>
struct Arguments {};

template <template <typename> auto Value,
          template <typename> concept Constraint>
int deduce_arguments(Arguments<Value, Constraint>)
  requires Constraint<int>
{
  return Value<int>;
}

template <template <typename> concept Constraint = HasSize,
          template <typename> auto Value = type_size,
          typename T = int>
  requires Constraint<T>
int use_defaults() {
  return Value<T>;
}

template <template <typename> concept... Constraints>
concept AllAccept = (Constraints<int> && ...);

int main() {
  if (deduce_arguments(Arguments<type_size, HasSize>{}) != sizeof(int)) {
    return 1;
  }
  if (use_defaults<>() != sizeof(int)) {
    return 2;
  }
  return AllAccept<HasSize, HasSize> ? 0 : 3;
}
