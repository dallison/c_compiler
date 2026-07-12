// RUN: -std=c++20
// EXPECT: Raw and numeric literal operator template cannot both be declared

int operator""_conflict(const char*) {
  return 1;
}

template<char... Chars>
int operator""_conflict() {
  return sizeof...(Chars);
}

int value = 123_conflict;
