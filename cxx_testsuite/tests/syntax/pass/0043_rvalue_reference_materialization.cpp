// RUN: -std=c++20

int read_ref(int&& value) {
  return value;
}

int main(void) {
  int value = 3;
  int&& literal_ref = 11;
  int&& sum_ref = value + 4;
  int&& cast_ref = static_cast<int&&>(value);
  return read_ref(5) + read_ref(value + 6) + literal_ref + sum_ref + cast_ref;
}
