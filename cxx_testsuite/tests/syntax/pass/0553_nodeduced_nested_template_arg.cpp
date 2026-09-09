// RUN: -std=c++20

template <class T>
struct Str {
  struct iter {};
};

template <class Iter, class Alloc>
struct Results {};

template <class T, class Alloc>
bool search(const Str<T>&, Results<typename Str<T>::iter, Alloc>&) {
  return true;
}

int main() {
  Str<char> input;
  Results<Str<char>::iter, int> found;
  return search(input, found) ? 0 : 1;
}
