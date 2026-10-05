// RUN: -std=c++20
// EXPECT_EXIT: 0

// char, signed char and unsigned char are three distinct types whatever the
// signedness of plain char, so specializations on each of them are separate
// entities with separate symbols.
template <class A, class B> struct Pair { static int id(); };
template <> int Pair<int, char>::id() { return 1; }
template <> int Pair<int, unsigned char>::id() { return 2; }
template <> int Pair<int, signed char>::id() { return 3; }

template <class T> int f(T) { return 10; }
template <> int f<char>(char) { return 11; }
template <> int f<unsigned char>(unsigned char) { return 12; }
template <> int f<signed char>(signed char) { return 13; }

int g(char) { return 21; }
int g(unsigned char) { return 22; }
int g(signed char) { return 23; }

template <class T> struct Is { static constexpr int v = 0; };
template <> struct Is<char> { static constexpr int v = 1; };
template <> struct Is<unsigned char> { static constexpr int v = 2; };
template <> struct Is<signed char> { static constexpr int v = 3; };

template <class T> struct Box {
  T v;
  int tag() const;
};
template <class T> int Box<T>::tag() const { return Is<T>::v; }

int main() {
  if (Pair<int, char>::id() != 1) return 1;
  if (Pair<int, unsigned char>::id() != 2) return 2;
  if (Pair<int, signed char>::id() != 3) return 3;
  if (f((char)1) != 11 || f((unsigned char)1) != 12 || f((signed char)1) != 13)
    return 4;
  if (g((char)1) != 21 || g((unsigned char)1) != 22 || g((signed char)1) != 23)
    return 5;
  Box<char> a;
  Box<unsigned char> b;
  Box<signed char> c;
  if (a.tag() != 1 || b.tag() != 2 || c.tag() != 3) return 6;
  return 0;
}
