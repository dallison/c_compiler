// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct Str {
  struct iter {};
};

template <class Iter, class Alloc>
struct Results {
  Alloc value;
};

template <class T, class Alloc>
bool search(const Str<T>&, Results<typename Str<T>::iter, Alloc>& results) {
  results.value = Alloc();
  return true;
}

int main() {
  Str<char> input;
  Results<Str<char>::iter, int> found;
  found.value = 7;
  if (!search(input, found)) {
    return 1;
  }
  if (found.value != 0) {
    return 2;
  }
  return 0;
}
