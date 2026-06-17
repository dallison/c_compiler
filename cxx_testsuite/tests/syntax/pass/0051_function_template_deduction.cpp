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

template <typename T, typename U>
T cast_first(U value) {
  return value;
}

template <typename T, typename U>
U choose_second(T first, U second) {
  return second;
}

template <int N>
int first_array_value(int (&values)[N]) {
  return values[0];
}

template <typename T, int N>
T first_typed_array_value(T (&values)[N]) {
  return values[0];
}

int main(void) {
  int first = identity(3);
  int second = add(4, 5);
  int third = declared_then_defined(6);
  Holder<int> wrapped = wrap_value(7);
  int casted = cast_first<int>(8);
  char small = 2;
  char chosen = choose_second<int>(1, small);
  int values[3];
  values[0] = 9;
  char letters[2];
  letters[0] = 1;
  int array_first = first_array_value(values);
  char typed_array_first = first_typed_array_value(letters);
  return first + second + third + wrapped.value + casted + chosen +
         array_first + typed_array_first;
}
