// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <new>
#include <utility>
#include <vector>

struct NonDefault {
  int value;

  explicit NonDefault(int v) : value(v) {}
  NonDefault(const NonDefault& other) : value(other.value) {}
  NonDefault(NonDefault&& other) : value(other.value) {
    other.value = -1;
  }
  NonDefault& operator=(const NonDefault& other) {
    value = other.value;
    return *this;
  }
  NonDefault& operator=(NonDefault&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
};

template <class T>
struct LocalAllocator {
  using value_type = T;
  using size_type = unsigned long;
  using difference_type = long;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  LocalAllocator() noexcept {}
  LocalAllocator(const LocalAllocator&) noexcept {}

  template <class U>
  LocalAllocator(const LocalAllocator<U>&) noexcept {}

  pointer allocate(size_type n) {
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  void deallocate(pointer ptr, size_type) noexcept {
    ::operator delete(static_cast<void*>(ptr));
  }

  void construct(T* ptr) {
    new (ptr) T();
  }

  void construct(T* ptr, const T& value) {
    new (ptr) T(value);
  }

  void construct(T* ptr, T&& value) {
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class U, class... Args>
  void construct(U* ptr, Args&&... args) {
    new (ptr) U(std::forward<Args>(args)...);
  }

  template <class U>
  void destroy(U* ptr) {
    ptr->~U();
  }

  size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T, class U>
bool operator==(const LocalAllocator<T>&, const LocalAllocator<U>&) {
  return true;
}

template <class T, class U>
bool operator!=(const LocalAllocator<T>&, const LocalAllocator<U>&) {
  return false;
}

int main(void) {
  std::vector<NonDefault, LocalAllocator<NonDefault> > values;
  values.push_back(NonDefault(4));
  values.push_back(NonDefault(9));
  if (values.size() != 2 || values[0].value != 4 || values[1].value != 9) {
    return 1;
  }
  return 0;
}
