// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <new>
#include <utility>

struct Source {
  int value;

  explicit Source(int v) : value(v) {
  }
};

struct Sink {
  int category;
  int value;

  Sink(Source& source) : category(1), value(source.value) {
  }

  Sink(Source&& source) : category(2), value(source.value) {
    source.value = -1;
  }
};

template <class T>
struct StorageMaker {
  template <class... Args>
  int construct_kind(Args&&... args) {
    T* storage = static_cast<T*>(::operator new(sizeof(T)));
    new (storage) T(std::forward<Args>(args)...);
    int result = storage->category * 100 + storage->value;
    storage->~T();
    ::operator delete(storage);
    return result;
  }
};

int main(void) {
  StorageMaker<Sink> maker;

  Source lvalue(11);
  if (maker.construct_kind(lvalue) != 111 || lvalue.value != 11) {
    return 1;
  }

  Source rvalue(22);
  if (maker.construct_kind(std::move(rvalue)) != 222 || rvalue.value != -1) {
    return 2;
  }

  return 0;
}
