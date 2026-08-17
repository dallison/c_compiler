// RUN: -std=c++20

#include <type_traits>

template <class T>
struct stable_container {
  T value;

  template <bool Const>
  struct cursor {
    using reference = std::conditional_t<Const, const T&, T&>;
    using pointer = std::conditional_t<Const, const T*, T*>;

    T* ptr;

    reference operator*() const { return *ptr; }
    pointer operator->() const { return ptr; }

    template <bool Other>
      requires(Const && !Other)
    cursor(const cursor<Other>& other) : ptr(other.ptr) {}

    cursor(T* value) : ptr(value) {}
  };

  using iterator = cursor<false>;
  using const_iterator = cursor<true>;
};

template <class Predicate>
void copy_iterator_in_function_template(stable_container<int>& values,
                                        Predicate) {
  stable_container<int>::iterator first(&values.value);
  stable_container<int>::iterator second = first;
  stable_container<int>::const_iterator constant = second;
  (void)constant;
}

void exercise(stable_container<int>& values) {
  stable_container<int>::iterator iterator(&values.value);
  stable_container<int>::iterator copied = iterator;
  stable_container<int>::const_iterator direct(iterator);
  stable_container<int>::const_iterator constant = iterator;
  int& mutable_reference = *iterator;
  const int& constant_reference = *constant;
  int* pointer = &*iterator;
  (void)mutable_reference;
  (void)constant_reference;
  (void)pointer;
  (void)copied;
  (void)direct;
  copy_iterator_in_function_template(values, [](int) { return true; });
}
