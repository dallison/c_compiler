// RUN: -std=c++23 -Werror=deprecated-declarations

enum First { first };
enum Second { second };

int left_array[2];
int right_array[2];

int combine(First lhs, Second rhs) {
  return lhs + rhs;
}

bool compare_arrays() {
  return left_array == right_array;
}

struct Incomplete;

void destroy(Incomplete* value) {
  delete value;
}

void old_variadic_spelling(int...);
