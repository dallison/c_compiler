// RUN: -std=c++17
// EXPECT_EXIT: 0

// A non-template constructor of the same arity must not hide a constructor
// template when the non-template cannot convert the argument.
// btree_set::equal_range builds iterator(const_iterator) while iterator(Node*)
// has that same arity.

template <class T>
struct Iter {
  Iter() : tag(1) {}
  explicit Iter(T*) : tag(2) {}

  template <class U>
  explicit Iter(const Iter<U>& other) : tag(other.tag + 4) {}

  int tag;
};

int main() {
  Iter<const int> from;
  Iter<int> to(from);
  return to.tag == 5 ? 0 : to.tag;
}
