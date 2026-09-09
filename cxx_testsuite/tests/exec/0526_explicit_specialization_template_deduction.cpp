// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct traits {};

template <>
struct traits<char> {};

template <class T, class Traits = traits<T>>
struct Str {};

template <class CharT>
bool take(const Str<CharT>&) {
  return true;
}

int main() {
  Str<char, traits<char>> value;
  if (!take(value)) {
    return 1;
  }
  return 0;
}
