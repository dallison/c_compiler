struct Box {
  int padding;

  int identity(int value) {
    return value + 10;
  }

  template <typename T>
  T identity(T value) {
    return value;
  }

  template <typename T>
  T* ptr_identity(T* value) {
    return value;
  }

  template <typename T>
  T ref_identity(T& value) {
    return value;
  }

  template <typename T, typename U>
  T first_as(U value) {
    return value;
  }

  template <typename T>
  T declared_then_defined(T value);
};

template <typename T>
T Box::declared_then_defined(T value) {
  return value + 1;
}

template <>
int Box::identity<int>(int value) {
  return value + 20;
}

template <>
int Box::declared_then_defined<int>(int value) {
  return value + 30;
}

int main(void) {
  Box box;
  box.padding = 1;
  Box* box_ptr = &box;
  char small = 3;
  int value = 7;
  int* pointer = &value;
  int concrete = box.identity(5);
  char templated = box.identity(small);
  int* same_pointer = box.ptr_identity(pointer);
  int by_ref = box.ref_identity(value);
  int explicit_int = box.identity<int>(4);
  char explicit_char = box.identity
      <char>(small);
  int explicit_arrow = box_ptr->identity<int>(2);
  int partial_explicit_member = box.first_as<int>(6);
  int out_of_class = box.declared_then_defined(8);
  int explicit_out_of_class = box.declared_then_defined<int>(9);
  int comparison = box.padding < 3;
  return concrete + templated + *same_pointer + by_ref + explicit_int +
         explicit_char + explicit_arrow + out_of_class +
         explicit_out_of_class + partial_explicit_member + comparison - 165;
}
