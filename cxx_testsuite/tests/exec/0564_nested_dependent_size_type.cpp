// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class A>
struct Traits {
  typedef unsigned long size_type;
};

template <class T, class A>
struct Holder {
  typedef typename Traits<A>::size_type size_type;
  struct Nested {
    unsigned long value;
    void annotate(size_type n);
  };
  Nested storage;
};

template <class T, class A>
void Holder<T, A>::Nested::annotate(typename Holder<T, A>::size_type n) {
  value = n;
}

int main() {
  Holder<int, char> h;
  h.storage.value = 0;
  h.storage.annotate(3);
  return h.storage.value == 3 ? 0 : 1;
}
