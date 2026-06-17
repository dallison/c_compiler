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

int main(void) {
  char small = 3;
  int* int_pointer = nullptr;
  char* char_pointer = nullptr;
  int concrete_select = select(int_pointer);
  int template_select = select(char_pointer);
  int template_select_again = select(char_pointer);
  int explicit_template_select = select<char>(char_pointer);
  int concrete_echo = echo(5);
  int template_echo = echo(small);
  int explicit_template_echo = echo<char>(small);
  return concrete_select + template_select + template_select_again +
         explicit_template_select + concrete_echo + template_echo +
         explicit_template_echo;
}
