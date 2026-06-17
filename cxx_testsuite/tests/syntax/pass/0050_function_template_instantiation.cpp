// RUN: -std=c++20

template <typename T>
struct Holder {
  T value;
};

template <int N>
struct Buffer {
  int data[N];
};

template <typename T>
T identity(T value) {
  return value;
}

template <>
int identity<int>(int value) {
  return value + 20;
}

template <typename T>
T declared_specialization(T value) {
  return value;
}

template <>
int declared_specialization<int>(int value);

template <>
int declared_specialization<int>(int value) {
  return value + 30;
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

template <typename T = int>
T default_value(void) {
  return 9;
}

template <typename T, typename U = Holder<T> >
U wrap_deduced_default(T value) {
  U result;
  result.value = value;
  return result;
}

template <typename T = int, typename U = Holder<T> >
U wrap_defaulted(T value) {
  U result;
  result.value = value;
  return result;
}

template <int N = 4>
Buffer<N> make_default_buffer(void) {
  Buffer<N> result;
  result.data[3] = 10;
  return result;
}

template <int N, int M = N>
Buffer<M> make_dependent_default_buffer(void) {
  Buffer<M> result;
  result.data[3] = 11;
  return result;
}

int main(void) {
  int first = identity<int>(3);
  int second = identity<int>(4);
  char small = identity<char>(2);
  int deduced_specialized = identity(5);
  int declared_specialized = declared_specialization(6);
  Holder<int> wrapped = wrap_value<int>(5);
  int defaulted = default_value<>();
  Holder<int> deduced_default = wrap_deduced_default(12);
  Holder<int> default_wrapped = wrap_defaulted<>(13);
  Holder<int> partial_default_wrapped = wrap_defaulted<int>(14);
  Buffer<4> default_buffer = make_default_buffer<>();
  Buffer<4> dependent_default_buffer = make_dependent_default_buffer<4>();
  return first + second + small + deduced_specialized + declared_specialized +
         add<int>(6, 7) + wrapped.value + declared_then_defined<int>(8) +
         defaulted + deduced_default.value + default_wrapped.value +
         partial_default_wrapped.value + default_buffer.data[3] +
         dependent_default_buffer.data[3];
}
