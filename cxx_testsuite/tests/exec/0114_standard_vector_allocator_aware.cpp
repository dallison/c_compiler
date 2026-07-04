// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <new>
#include <utility>
#include <vector>

struct AllocCounts {
  static int allocations;
  static int deallocations;
  static int constructs;
  static int destroys;
};

int AllocCounts::allocations = 0;
int AllocCounts::deallocations = 0;
int AllocCounts::constructs = 0;
int AllocCounts::destroys = 0;

struct Payload {
  int first;
  int second;

  Payload(int a, int b) : first(a), second(b) {}
  Payload(const Payload& other) : first(other.first), second(other.second) {}
  Payload(Payload&& other) : first(other.first), second(other.second) {
    other.first = -1;
    other.second = -1;
  }
  Payload& operator=(const Payload& other) {
    first = other.first;
    second = other.second;
    return *this;
  }
  Payload& operator=(Payload&& other) {
    first = other.first;
    second = other.second;
    other.first = -1;
    other.second = -1;
    return *this;
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
    ++AllocCounts::allocations;
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    ++AllocCounts::deallocations;
    ::operator delete(static_cast<void*>(ptr));
  }

  void construct(pointer ptr, const T& value) {
    ++AllocCounts::constructs;
    new (ptr) T(value);
  }

  void construct(pointer ptr, T&& value) {
    ++AllocCounts::constructs;
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    ++AllocCounts::constructs;
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    ++AllocCounts::destroys;
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

int main(void) {
  {
    std::vector<Payload, CountingAllocator<Payload> > values;
    values.reserve(2);
    if (AllocCounts::allocations == 0 || AllocCounts::constructs != 0) {
      return 1;
    }

    values.emplace_back(1, 2);
    Payload copy_source(3, 4);
    values.push_back(copy_source);
    values.push_back(Payload(5, 6));
    values.insert(values.begin() + 1, Payload(7, 8));
    values.emplace(values.begin() + 2, 9, 10);

    if (values.size() != 5) {
      return 2;
    }
    if (values[0].first != 1 || values[1].first != 7 ||
        values[2].first != 9 || values[3].first != 3 ||
        values[4].first != 5) {
      return 3;
    }
    if (AllocCounts::constructs == 0 || AllocCounts::destroys == 0) {
      return 4;
    }

    values.erase(values.begin() + 1);
    values.clear();
    if (!values.empty()) {
      return 5;
    }
  }

  if (AllocCounts::constructs != AllocCounts::destroys) {
    return 6;
  }
  if (AllocCounts::allocations != AllocCounts::deallocations) {
    return 7;
  }
  return 0;
}
