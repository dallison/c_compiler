// RUN: -std=c++20

int select(int& value) {
  return value + 1;
}

int select(const int& value) {
  return value + 2;
}

int select(int&& value) {
  return value + 3;
}

int fallback(const int& value) {
  return value + 4;
}

int main(void) {
  int value = 7;
  const int const_value = 8;
  return select(value) + select(const_value) + select(9) +
         select(value + 1) + select(static_cast<int&&>(value)) +
         fallback(10);
}
