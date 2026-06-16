// RUN: -std=c++20

template <typename T>
struct Holder {
  T value;
};

template <typename T>
T identity(T value) {
  return value;
}

template <typename T>
T add(T left, T right) {
  return left + right;
}

template <typename T>
T declared_then_defined(T value);

template <typename T>
T declared_then_defined(T value) {
  return value + 1;
}

template <typename T>
Holder<T> wrap_value(T value) {
  Holder<T> result;
  result.value = value;
  return result;
}

int main(void) {
  int first = identity<int>(3);
  int second = identity<int>(4);
  char small = identity<char>(2);
  Holder<int> wrapped = wrap_value<int>(5);
  return first + second + small + add<int>(6, 7) + wrapped.value +
         declared_then_defined<int>(8);
}
