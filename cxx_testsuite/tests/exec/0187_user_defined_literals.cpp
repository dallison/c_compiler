// RUN: -std=c++20
// EXPECT_EXIT: 0

int operator""_twice(unsigned long long value) {
  return (int)(value * 2);
}

int operator""_raw(const char* text) {
  int count = 0;
  while (text[count] != 0) {
    count++;
  }
  return count;
}

int operator""_fraw(const char* text) {
  int count = 0;
  while (text[count] != 0) {
    count++;
  }
  return count;
}

int operator "" _ch(char value) {
  return value + 1;
}

int operator""_len(const char* text, unsigned long size) {
  return (int)(text[0] + size);
}

int operator""s(unsigned long long value) {
  return (int)value + 1000;
}

int operator""us(unsigned long long value) {
  return (int)value + 2000;
}

int operator""cstr(const char* text, unsigned long size) {
  return (int)(text[0] + text[1] + size);
}

int main() {
  if (21_twice != 42) {
    return 1;
  }
  if (12345_raw != 5) {
    return 2;
  }
  if (1.25_fraw != 4) {
    return 3;
  }
  if ('A'_ch != 'B') {
    return 4;
  }
  if ("abc"_len != 'a' + 3) {
    return 5;
  }
  if (1s != 1001) {
    return 6;
  }
  if (1us != 2001) {
    return 7;
  }
  if ("a" "b"cstr != 'a' + 'b' + 2) {
    return 8;
  }
  return 0;
}
