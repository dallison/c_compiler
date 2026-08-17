// RUN: -std=c++26

const char* basic_characters = "$@`";
const char* raw_with_basic_delimiter = R"$@`(contents)$@`";

int \u039\
3 = 1;

#define JOIN(left, right) left ## right
int JOIN(\,u0394) = 2;

void shift_assign(int& value) {
  value JOIN(>,>=) 1;
}
