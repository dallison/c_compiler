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

  if (select(value) != 8) {
    return 1;
  }
  if (select(const_value) != 10) {
    return 2;
  }
  if (select(9) != 12) {
    return 3;
  }
  if (select(value + 1) != 11) {
    return 4;
  }
  if (select(static_cast<int&&>(value)) != 10) {
    return 5;
  }
  if (fallback(10) != 14) {
    return 6;
  }
  return 0;
}
