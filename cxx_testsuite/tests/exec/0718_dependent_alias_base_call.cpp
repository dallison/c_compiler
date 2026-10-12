// RUN: -std=c++17
// EXPECT_EXIT: 0

struct Fallback {};

template <class T>
struct Callable {
  int operator()(T value) const {
    return value + 2;
  }
};

template <class T>
struct IsCallable {
  static constexpr bool value = true;
};

template <bool Condition, class TrueType, class FalseType>
struct Conditional {
  using type = TrueType;
};

template <class TrueType, class FalseType>
struct Conditional<false, TrueType, FalseType> {
  using type = FalseType;
};

template <bool Condition, class TrueType, class FalseType>
using ConditionalType =
    typename Conditional<Condition, TrueType, FalseType>::type;

template <class T>
struct Hash
    : ConditionalType<IsCallable<T>::value, Callable<T>, Fallback> {};

template <class T>
int CallHash(T value) {
  return Hash<T>{}(value);
}

int main() {
  return CallHash(40) == 42 ? 0 : 1;
}
