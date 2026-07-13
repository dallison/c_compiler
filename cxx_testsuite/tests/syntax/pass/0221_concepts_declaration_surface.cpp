// RUN: -std=c++20

template <typename T>
concept NonEmpty = sizeof(T) > 0;

template <typename T>
  requires NonEmpty<T>
struct Box {
  T value;

  void reset() requires NonEmpty<T> {
    value = T();
  }

  template <NonEmpty U>
  U convert(U input) {
    return input;
  }
};

template <NonEmpty auto N>
struct Constant {
  static constexpr auto value = N;
};

template <typename T>
  requires NonEmpty<T>
constexpr int variable_value = sizeof(T);

template <typename T>
  requires NonEmpty<T>
using Alias = T;

NonEmpty auto constrained_value = 7;

static_assert(Constant<3>::value == 3);
static_assert(variable_value<int> == sizeof(int));
static_assert(sizeof(Alias<long>) == sizeof(long));

int main() {
  Box<int> box{1};
  box.reset();
  return box.convert(constrained_value);
}
