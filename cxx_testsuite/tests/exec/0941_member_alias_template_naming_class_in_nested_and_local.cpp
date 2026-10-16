// RUN: -std=c++17
// EXPECT_EXIT: 0

// `Traits<Alloc>::template rebind_traits<T>::const_pointer`, where the member
// alias template names a class (`Traits<typename Alloc::rebind<T>::other>`),
// resolves to the rebound allocator's member in a nested class's members and
// in a local variable of a member function, without an `allocator_type` in
// the enclosing class.
template <class T>
struct Allocator {
  using const_pointer = const T*;
  template <class U>
  struct rebind {
    using other = Allocator<U>;
  };
};

template <class Alloc>
struct Traits {
  using const_pointer = typename Alloc::const_pointer;
  template <class T>
  using rebind_traits = Traits<typename Alloc::template rebind<T>::other>;
};

template <class T, class Alloc>
struct Container {
  using const_pointer =
      typename Traits<Alloc>::template rebind_traits<T>::const_pointer;
  struct Iterator {
    explicit Iterator(const_pointer p) : pointer(p) {}
    const_pointer pointer;
  };
  const T* data;
  Iterator begin() const { return Iterator(data); }
  long local() const {
    const_pointer p = data;
    return *p + static_cast<long>(sizeof(p));
  }
};

int main() {
  long value = 40;
  Container<long, Allocator<char>> c{&value};
  if (*c.begin().pointer != 40) return 1;
  if (c.local() != 40 + static_cast<long>(sizeof(long*))) return 2;
  return 0;
}
