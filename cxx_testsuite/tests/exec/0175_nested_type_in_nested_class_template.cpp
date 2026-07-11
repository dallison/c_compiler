// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct Box {
  struct iterator {
    T* ptr;

    iterator() : ptr(nullptr) {}
    explicit iterator(T* p) : ptr(p) {}
  };

  struct result {
    typename Box::iterator position;

    explicit result(typename Box::iterator pos) : position(pos) {}
  };

  T value;

  result make() {
    typename Box::iterator it(&value);
    return result(it);
  }
};

int main() {
  Box<int> box;
  box.value = 42;
  Box<int>::result result = box.make();
  return result.position.ptr != nullptr && *result.position.ptr == 42 ? 0 : 1;
}
