// RUN: -std=c++20

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
  return take(value) ? 0 : 1;
}
