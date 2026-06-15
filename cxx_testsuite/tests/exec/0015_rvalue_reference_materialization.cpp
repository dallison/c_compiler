int read_ref(int&& value) {
  value = value + 1;
  return value;
}

int main(void) {
  int value = 3;
  int&& literal_ref = 11;
  if (literal_ref != 11) {
    return 1;
  }
  literal_ref = 13;
  if (literal_ref != 13) {
    return 2;
  }

  int&& sum_ref = value + 4;
  if (sum_ref != 7) {
    return 3;
  }
  sum_ref = 19;
  if (value != 3) {
    return 4;
  }

  int&& cast_ref = static_cast<int&&>(value);
  cast_ref = 17;
  if (value != 17) {
    return 5;
  }

  if (read_ref(5) != 6) {
    return 6;
  }
  if (read_ref(value + 6) != 24) {
    return 7;
  }
  return 0;
}
