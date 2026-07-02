// RUN: -std=c++20
#include <iterator>

template <class T>
struct HasReverseIterator {
  using iterator = T*;
  using reverse_iterator = std::reverse_iterator<iterator>;

  T* data;
  unsigned long size;

  reverse_iterator alias_rbegin() {
    return reverse_iterator(data + size);
  }

  reverse_iterator explicit_rbegin() {
    return std::reverse_iterator<iterator>(data + size);
  }
};

struct Element {
  int value;
};

int main(void) {
  int ints[2] = {1, 2};
  HasReverseIterator<int> int_range;
  int_range.data = ints;
  int_range.size = 2;
  (void)int_range.alias_rbegin();
  (void)int_range.explicit_rbegin();

  Element elements[2] = {{1}, {2}};
  HasReverseIterator<Element> element_range;
  element_range.data = elements;
  element_range.size = 2;
  (void)element_range.alias_rbegin();
  (void)element_range.explicit_rbegin();
  return 0;
}
