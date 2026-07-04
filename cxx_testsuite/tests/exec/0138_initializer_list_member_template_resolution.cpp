// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <initializer_list>
#include <new>

struct ListBox {
  int sum;
  int extra;

  ListBox(std::initializer_list<int> values, int e) : sum(0), extra(e) {
    for (int value : values) {
      sum += value;
    }
  }
};

template <class T>
struct Holder {
  union Storage {
    char dummy;
    T value;
  };

  Storage storage;

  T* ptr() {
    return &storage.value;
  }

  template <class... Args>
  T& emplace(Args&&... args) {
    new (ptr()) T(static_cast<Args&&>(args)...);
    return *ptr();
  }

  template <class U, class... Args>
  T& emplace(std::initializer_list<U> init, Args&&... args) {
    new (ptr()) T(init, static_cast<Args&&>(args)...);
    return *ptr();
  }
};

int main(void) {
  Holder<int> ints;
  if (ints.emplace(7) != 7) {
    return 1;
  }

  Holder<ListBox> lists;
  ListBox& box = lists.emplace({1, 2, 3}, 4);
  if (box.sum != 6 || box.extra != 4) {
    return 2;
  }

  return 0;
}
