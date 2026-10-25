// RUN: -std=c++17
// EXPECT_EXIT: 0

// `using typename Base::type` introduces the nested type in a class template.

template <class T>
struct Base {
  struct iterator {
    T value;
  };
  using const_iterator = const T*;
};

template <class T>
struct Derived : Base<T> {
  using Vector = Base<T>;
  using typename Vector::iterator;
  using typename Vector::const_iterator;

  iterator make(T v) {
    iterator it;
    it.value = v;
    return it;
  }
  const_iterator empty() const { return 0; }
};

int main() {
  Derived<int> d;
  return d.make(4).value == 4 && d.empty() == 0 ? 0 : 1;
}
