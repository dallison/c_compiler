// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class Iter>
struct ReverseLike {
  Iter current;

  explicit ReverseLike(Iter iter) : current(iter) {
  }

  Iter base(void) const {
    return current;
  }
};

template <class T>
struct Range {
  using iterator = T*;
  using reverse_iterator = ReverseLike<iterator>;

  T* data;
  unsigned long size;

  reverse_iterator rbegin(void) {
    return reverse_iterator(data + size);
  }
};

int main(void) {
  int values[3] = {1, 2, 3};
  int* base = values;
  Range<int> range;
  range.data = base;
  range.size = 3;

  Range<int>::reverse_iterator iter = range.rbegin();
  return iter.base() - base == 3 ? 0 : 1;
}
