struct HasNestedType {
  int value;
  struct type {
    int value;
  };
};

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
  T sfinae_same(T left, T right) {
    return left + right + 100;
  }

  int sfinae_same(int left, char right) {
    return left + right + 200;
  }

  template <typename T, typename U>
  U sfinae_undeduced(T value) {
    return value;
  }

  int sfinae_undeduced(int value) {
    return value + 400;
  }

  template <typename T>
  typename T::type sfinae_member_type(T value) {
    typename T::type result;
    result.value = value.value + 500;
    return result;
  }

  int sfinae_member_type(int value) {
    return value + 600;
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
  HasNestedType nested_type = {7};
  int concrete = box.identity(5);
  char templated = box.identity(small);
  int* same_pointer = box.ptr_identity(pointer);
  int by_ref = box.ref_identity(value);
  int explicit_int = box.identity<int>(4);
  char explicit_char = box.identity
      <char>(small);
  int explicit_arrow = box_ptr->identity<int>(2);
  int partial_explicit_member = box.first_as<int>(6);
  int sfinae_fallback = box.sfinae_same(1, 'c');
  int sfinae_undeduced_fallback = box.sfinae_undeduced(5);
  int sfinae_member_type_fallback = box.sfinae_member_type(5);
  typename HasNestedType::type sfinae_member_type_template =
      box.sfinae_member_type(nested_type);
  int out_of_class = box.declared_then_defined(8);
  int explicit_out_of_class = box.declared_then_defined<int>(9);
  int comparison = box.padding < 3;
  return concrete + templated + *same_pointer + by_ref + explicit_int +
         explicit_char + explicit_arrow + out_of_class +
         explicit_out_of_class + partial_explicit_member + sfinae_fallback +
         sfinae_undeduced_fallback + sfinae_member_type_fallback +
         sfinae_member_type_template.value + comparison - 1982;
}
