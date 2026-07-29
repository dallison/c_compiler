// RUN: -std=c++20
// EXPECT_EXIT: 0

int pick(char) { return 1; }
int pick(unsigned char) { return 2; }
int pick(char8_t) { return 3; }

unsigned long operator""_u8_units(const char8_t* text, unsigned long length) {
  return length + (unsigned long)text[0];
}

template <class T>
struct type_tag {
  static int value() { return 0; }
};

template <>
struct type_tag<char8_t> {
  static int value() { return 8; }
};

template <class T>
int deduce_value(T) {
  return type_tag<T>::value();
}

template <class T, unsigned long N>
int inspect_literal(const T (&text)[N]) {
  return type_tag<T>::value() + (int)N + (int)text[0];
}

int main() {
  char8_t value = u8'A';
  if (sizeof(value) != 1 || value != char8_t{65}) return 1;
  if (pick(u8'x') != 3) return 2;
  if (deduce_value(u8'x') != 8) return 3;
  if (inspect_literal(u8"ab") != 8 + 3 + 'a') return 4;
  if (u8"ok"_u8_units != 2 + 'o') return 11;

  const char8_t text[] = u8"hello";
  if (text[0] != u8'h' || text[4] != u8'o' || text[5] != 0) return 5;

  const char8_t copyright[] = u8"\u00a9";
  if (sizeof(copyright) != 3) return 6;
  if ((unsigned char)copyright[0] != 0xc2 ||
      (unsigned char)copyright[1] != 0xa9 ||
      copyright[2] != 0) {
    return 7;
  }

  const char8_t smile[] = u8"\U0001f642";
  if (sizeof(smile) != 5) return 8;
  if ((unsigned char)smile[0] != 0xf0 ||
      (unsigned char)smile[1] != 0x9f ||
      (unsigned char)smile[2] != 0x99 ||
      (unsigned char)smile[3] != 0x82 ||
      smile[4] != 0) {
    return 9;
  }

  unsigned char round_trip = (unsigned char)value;
  if ((char8_t)round_trip != value) return 10;
  return 0;
}
