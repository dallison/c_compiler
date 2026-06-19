// RUN: -std=c++20

int select(int* value) {
  return sizeof(value) + 100;
}

template <typename T>
int select(T* value) {
  return sizeof(value) + 200;
}

template <typename T>
T echo(T value) {
  return value;
}

int echo(int value) {
  return value + 10;
}

struct HasNestedType {
  int value;
  struct type {
    int value;
  };
};

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

int main(void) {
  char small = 3;
  int* int_pointer = nullptr;
  char* char_pointer = nullptr;
  HasNestedType nested_type = {7};
  int concrete_select = select(int_pointer);
  int template_select = select(char_pointer);
  int template_select_again = select(char_pointer);
  int explicit_template_select = select<char>(char_pointer);
  int concrete_echo = echo(5);
  int template_echo = echo(small);
  int explicit_template_echo = echo<char>(small);
  int sfinae_fallback = sfinae_same(1, 'c');
  int sfinae_undeduced_fallback = sfinae_undeduced(5);
  int sfinae_member_type_fallback = sfinae_member_type(5);
  typename HasNestedType::type sfinae_member_type_template =
      sfinae_member_type(nested_type);
  return concrete_select + template_select + template_select_again +
         explicit_template_select + concrete_echo + template_echo +
         explicit_template_echo + sfinae_fallback + sfinae_undeduced_fallback +
         sfinae_member_type_fallback + sfinae_member_type_template.value;
}
