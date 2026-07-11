// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct Box {
  struct iterator {
    T* ptr;
    T* root;

    iterator() : ptr(nullptr), root(nullptr) {}
    iterator(T* p, T* r) : ptr(p), root(r) {}
  };

  T value;

  template <class K>
  T* find_as(const K& key) {
    return key == value ? &value : nullptr;
  }

  template <class K>
  iterator find(const K& key) {
    return iterator(find_as(key), &value);
  }
};

int main() {
  Box<int> box;
  box.value = 7;
  Box<int>::iterator it = box.find(7);
  return it.root != nullptr ? 0 : 1;
}
