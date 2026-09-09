// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T, class Traits = int, class Alloc = int>
struct Vec {};

template <class T>
bool take_defaulted(const Vec<T>&) {
  return true;
}

template <class CharT, class Traits = CharT*>
struct Str {};

template <class CharT>
bool take_dependent_default(const Str<CharT>&) {
  return true;
}

int main() {
  Vec<char, int> v;
  if (!take_defaulted(v)) {
    return 1;
  }
  Str<char, char*> s;
  if (!take_dependent_default(s)) {
    return 2;
  }
  return 0;
}
