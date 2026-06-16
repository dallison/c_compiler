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
  int first = identity(3);
  int second = add(4, 5);
  int third = declared_then_defined(6);
  Holder<int> wrapped = wrap_value(7);
  return first + second + third + wrapped.value;
}
