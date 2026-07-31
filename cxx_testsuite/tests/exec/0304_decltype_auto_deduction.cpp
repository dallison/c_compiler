// RUN: -std=c++20
// EXPECT_EXIT: 0

struct box {
  int value;
};

int global_value = 1;
int other_value = 2;

decltype(auto) return_id() {
  return global_value;
}

decltype(auto) return_reference_id(int& value) {
  return value;
}

decltype(auto) return_parenthesized_id() {
  return (global_value);
}

decltype(auto) return_member(box& value) {
  return value.value;
}

decltype(auto) return_parenthesized_member(box& value) {
  return (value.value);
}

decltype(auto) return_arrow_member(box* value) {
  return value->value;
}

decltype(auto) return_subscript(int* value) {
  return value[0];
}

decltype(auto) return_xvalue() {
  return static_cast<int&&>(global_value);
}

decltype(auto) select_global(bool first) {
  if (first) {
    return (global_value);
  }
  return (other_value);
}

template <class T>
decltype(auto) return_template_reference(T& value) {
  return value;
}

int main() {
  int source = 3;
  int& source_reference = source;
  box object{4};
  box* pointer = &object;

  decltype(auto) id_copy = source;
  decltype(auto) id_reference = source_reference;
  decltype(auto) parenthesized_reference = (source);
  decltype(auto) member_copy = object.value;
  decltype(auto) member_reference = (object.value);
  decltype(auto) arrow_copy = pointer->value;

  id_copy = 10;
  member_copy = 11;
  arrow_copy = 12;
  if (source != 3 || object.value != 4) {
    return 1;
  }

  id_reference = 20;
  if (source != 20) {
    return 2;
  }
  parenthesized_reference = 21;
  if (source != 21) {
    return 3;
  }
  member_reference = 22;
  if (object.value != 22) {
    return 4;
  }

  global_value = 30;
  decltype(auto) returned_copy = return_id();
  returned_copy = 31;
  if (global_value != 30) {
    return 5;
  }
  return_reference_id(global_value) = 32;
  if (global_value != 32) {
    return 6;
  }
  return_parenthesized_id() = 33;
  if (global_value != 33) {
    return 7;
  }

  decltype(auto) returned_member_copy = return_member(object);
  returned_member_copy = 40;
  if (object.value != 22) {
    return 8;
  }
  return_parenthesized_member(object) = 41;
  if (object.value != 41) {
    return 9;
  }
  decltype(auto) returned_arrow_copy = return_arrow_member(pointer);
  returned_arrow_copy = 42;
  if (object.value != 41) {
    return 10;
  }

  int values[] = {50};
  return_subscript(values) = 51;
  if (values[0] != 51) {
    return 11;
  }
  return_xvalue() = 52;
  if (global_value != 52) {
    return 12;
  }
  select_global(false) = 53;
  if (other_value != 53) {
    return 13;
  }
  return_template_reference(source) = 54;
  return source == 54 ? 0 : 14;
}
