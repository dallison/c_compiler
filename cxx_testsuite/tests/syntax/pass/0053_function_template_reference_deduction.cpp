// RUN: -std=c++20

template <typename T>
T* ptr_identity(T* value) {
  return value;
}

template <typename T>
T ref_identity(T& value) {
  return value;
}

template <typename T>
T const_ref_identity(const T& value) {
  return value;
}

template <typename T>
T trailing_const_ref_identity(T const& value) {
  return value;
}

int main(void) {
  int value = 7;
  int* pointer = &value;
  int* same_pointer = ptr_identity(pointer);
  int by_ref = ref_identity(value);
  const int const_value = 11;
  int by_const_ref = const_ref_identity(const_value);
  int literal_by_const_ref = const_ref_identity(13);
  int by_trailing_const_ref = trailing_const_ref_identity(const_value);
  return *same_pointer + by_ref + by_const_ref + literal_by_const_ref +
         by_trailing_const_ref;
}
