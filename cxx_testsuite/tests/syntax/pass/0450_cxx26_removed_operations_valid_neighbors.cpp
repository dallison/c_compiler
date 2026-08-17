// RUN: -std=c++26 -Werror=deprecated-declarations

enum Value { value };

int left_array[2];
int right_array[2];

int enum_and_integer(Value input) {
  return input + 1;
}

bool array_and_pointer() {
  int* pointer = right_array;
  return left_array == pointer;
}

struct Complete {};

void destroy_complete(Complete* input) {
  delete input;
}

void comma_separated_variadic(int, ...);
void ellipsis_only(...);
