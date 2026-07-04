// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <memory>
#include <new>
#include <utility>

struct Counts {
  static int allocations;
  static int deallocations;
  static int constructs;
  static int destroys;
};

int Counts::allocations = 0;
int Counts::deallocations = 0;
int Counts::constructs = 0;
int Counts::destroys = 0;

struct Payload {
  int first;
  int second;
  int category;

  Payload(int a, int b) : first(a), second(b), category(1) {}
  Payload(const Payload& other)
      : first(other.first), second(other.second), category(2) {}
  Payload(Payload&& other)
      : first(other.first), second(other.second), category(3) {
    other.first = -1;
    other.second = -1;
  }
};

template <class T>
struct CountingAllocator {
  using value_type = T;
  using size_type = unsigned long;
  using difference_type = long;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  CountingAllocator() noexcept {}
  CountingAllocator(const CountingAllocator&) noexcept {}

  template <class U>
  CountingAllocator(const CountingAllocator<U>&) noexcept {}

  pointer allocate(size_type n) {
    ++Counts::allocations;
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    ++Counts::deallocations;
    ::operator delete(static_cast<void*>(ptr));
  }

  void construct(pointer ptr, const T& value) {
    ++Counts::constructs;
    new (ptr) T(value);
  }

  void construct(pointer ptr, T&& value) {
    ++Counts::constructs;
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    ++Counts::constructs;
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    ++Counts::destroys;
    ptr->~T();
  }

  size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T, class U>
bool operator==(const CountingAllocator<T>&, const CountingAllocator<U>&) {
  return true;
}

template <class T, class U>
bool operator!=(const CountingAllocator<T>&, const CountingAllocator<U>&) {
  return false;
}

template <class Alloc>
struct TraitsUser {
  using traits_type = std::allocator_traits<Alloc>;
  using pointer = typename traits_type::pointer;
  using size_type = typename traits_type::size_type;

  Alloc alloc;

  pointer allocate(size_type count) {
    return traits_type::allocate(alloc, count);
  }

  void deallocate(pointer ptr, size_type count) {
    traits_type::deallocate(alloc, ptr, count);
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    traits_type::construct(alloc, ptr, std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    traits_type::destroy(alloc, ptr);
  }

  size_type max_size(void) const {
    return traits_type::max_size(alloc);
  }
};

int main(void) {
  TraitsUser<CountingAllocator<Payload> > user;
  Payload* storage = user.allocate(3);
  if (Counts::allocations != 1 || user.max_size() == 0) {
    return 1;
  }

  user.construct(storage, 1, 2);
  Payload source(3, 4);
  user.construct(storage + 1, source);
  user.construct(storage + 2, Payload(5, 6));

  if (storage[0].first != 1 || storage[0].second != 2 ||
      storage[0].category != 1) {
    return 2;
  }
  if (storage[1].first != 3 || storage[1].second != 4 ||
      storage[1].category != 2) {
    return 3;
  }
  if (storage[2].first != 5 || storage[2].second != 6 ||
      storage[2].category != 3) {
    return 4;
  }
  if (Counts::constructs != 3) {
    return 5;
  }

  user.destroy(storage + 2);
  user.destroy(storage + 1);
  user.destroy(storage);
  user.deallocate(storage, 3);

  if (Counts::destroys != 3 || Counts::deallocations != 1) {
    return 6;
  }
  return 0;
}
