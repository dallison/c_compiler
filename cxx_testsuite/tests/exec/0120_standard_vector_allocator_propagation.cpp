// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <memory>
#include <new>
#include <utility>

int tracking_allocations = 0;
int tracking_deallocations = 0;
int tracking_bad_deallocate = 0;
int tracking_constructs = 0;
int tracking_destroys = 0;

void reset_tracking(void) {
  tracking_allocations = 0;
  tracking_deallocations = 0;
  tracking_bad_deallocate = 0;
  tracking_constructs = 0;
  tracking_destroys = 0;
}

struct Payload {
  int value;

  Payload() : value(0) {}
  explicit Payload(int v) : value(v) {}
  Payload(const Payload& other) : value(other.value) {}
  Payload(Payload&& other) : value(other.value) {
    other.value = -1;
  }
  Payload& operator=(const Payload& other) {
    value = other.value;
    return *this;
  }
  Payload& operator=(Payload&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
};

template <class T>
struct NonPropagatingAllocator {
  using value_type = T;
  using size_type = unsigned long;
  using difference_type = long;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::false_type;
  using is_always_equal = std::false_type;

  int id;

  NonPropagatingAllocator() noexcept : id(0) {}
  explicit NonPropagatingAllocator(int value) noexcept : id(value) {}
  NonPropagatingAllocator(const NonPropagatingAllocator& other) noexcept
      : id(other.id) {}

  template <class U>
  NonPropagatingAllocator(const NonPropagatingAllocator<U>& other) noexcept
      : id(other.id) {}

  pointer allocate(size_type n) {
    ++tracking_allocations;
    char* raw = static_cast<char*>(::operator new(n * sizeof(T) + sizeof(int)));
    int* header = reinterpret_cast<int*>(raw);
    *header = id;
    return reinterpret_cast<pointer>(raw + sizeof(int));
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    ++tracking_deallocations;
    char* raw = reinterpret_cast<char*>(ptr);
    raw = raw - sizeof(int);
    int* header = reinterpret_cast<int*>(raw);
    if (*header != id) {
      tracking_bad_deallocate = *header * 1000 + id;
    }
    ::operator delete(static_cast<void*>(raw));
  }

  void construct(pointer ptr, const T& value) {
    ++tracking_constructs;
    new (ptr) T(value);
  }

  void construct(pointer ptr, T&& value) {
    ++tracking_constructs;
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    ++tracking_constructs;
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    ++tracking_destroys;
    ptr->~T();
  }

  size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T, class U>
bool operator==(const NonPropagatingAllocator<T>& left,
                const NonPropagatingAllocator<U>& right) {
  return left.id == right.id;
}

template <class T, class U>
bool operator!=(const NonPropagatingAllocator<T>& left,
                const NonPropagatingAllocator<U>& right) {
  return !(left == right);
}

template <class T>
struct PropagatingAllocator {
  using value_type = T;
  using size_type = unsigned long;
  using difference_type = long;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  using is_always_equal = std::false_type;

  int id;

  PropagatingAllocator() noexcept : id(0) {}
  explicit PropagatingAllocator(int value) noexcept : id(value) {}
  PropagatingAllocator(const PropagatingAllocator& other) noexcept
      : id(other.id) {}

  template <class U>
  PropagatingAllocator(const PropagatingAllocator<U>& other) noexcept
      : id(other.id) {}

  pointer allocate(size_type n) {
    ++tracking_allocations;
    char* raw = static_cast<char*>(::operator new(n * sizeof(T) + sizeof(int)));
    int* header = reinterpret_cast<int*>(raw);
    *header = id;
    return reinterpret_cast<pointer>(raw + sizeof(int));
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    ++tracking_deallocations;
    char* raw = reinterpret_cast<char*>(ptr);
    raw = raw - sizeof(int);
    int* header = reinterpret_cast<int*>(raw);
    if (*header != id) {
      tracking_bad_deallocate = *header * 1000 + id;
    }
    ::operator delete(static_cast<void*>(raw));
  }

  void construct(pointer ptr, const T& value) {
    ++tracking_constructs;
    new (ptr) T(value);
  }

  void construct(pointer ptr, T&& value) {
    ++tracking_constructs;
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    ++tracking_constructs;
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    ++tracking_destroys;
    ptr->~T();
  }

  size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T, class U>
bool operator==(const PropagatingAllocator<T>& left,
                const PropagatingAllocator<U>& right) {
  return left.id == right.id;
}

template <class T, class U>
bool operator!=(const PropagatingAllocator<T>& left,
                const PropagatingAllocator<U>& right) {
  return !(left == right);
}

#include <vector>

int test_non_propagating_assignment(void) {
  using Alloc = NonPropagatingAllocator<Payload>;
  using Vec = std::vector<Payload, Alloc>;

  reset_tracking();
  {
    Vec left(Alloc(1));
    left.emplace_back(1);
    Vec right(Alloc(2));
    right.emplace_back(2);
    right.emplace_back(3);

    left = right;
    if (left.get_allocator().id != 1 || right.get_allocator().id != 2) {
      return 1;
    }
    if (left.size() != 2 || left[0].value != 2 || left[1].value != 3) {
      return 2;
    }

    Vec moved(Alloc(3));
    moved.emplace_back(4);
    moved.emplace_back(5);
    left = static_cast<Vec&&>(moved);
    if (left.get_allocator().id != 1 || moved.get_allocator().id != 3) {
      return 3;
    }
    if (left.size() != 2 || left[0].value != 4 || left[1].value != 5) {
      return 4;
    }
    if (!moved.empty()) {
      return 5;
    }
    moved.emplace_back(6);
    if (moved.size() != 1 || moved[0].value != 6) {
      return 6;
    }
  }
  if (tracking_bad_deallocate != 0) {
    return 7;
  }
  if (tracking_allocations != tracking_deallocations ||
      tracking_constructs != tracking_destroys) {
    return 8;
  }

  reset_tracking();
  {
    Vec left(Alloc(10));
    left.emplace_back(1);
    Vec right(Alloc(20));
    right.emplace_back(2);
    right.emplace_back(3);

    left.swap(right);
    if (left.get_allocator().id != 10 || right.get_allocator().id != 20) {
      return 20;
    }
    if (left.size() != 2 || left[0].value != 2 || left[1].value != 3) {
      return 21;
    }
    if (right.size() != 1 || right[0].value != 1) {
      return 22;
    }
  }
  if (tracking_bad_deallocate != 0) {
    return 23;
  }
  if (tracking_allocations != tracking_deallocations ||
      tracking_constructs != tracking_destroys) {
    return 24;
  }
  return 0;
}

int test_propagating_assignment_and_swap(void) {
  using Alloc = PropagatingAllocator<Payload>;
  using Vec = std::vector<Payload, Alloc>;

  reset_tracking();
  {
    Vec left(Alloc(1));
    left.emplace_back(1);
    Vec right(Alloc(2));
    right.emplace_back(2);
    right.emplace_back(3);

    left = right;
    if (left.get_allocator().id != 2 || right.get_allocator().id != 2) {
      return 30;
    }
    if (left.size() != 2 || left[0].value != 2 || left[1].value != 3) {
      return 31;
    }

    Vec moved(Alloc(3));
    moved.emplace_back(4);
    moved.emplace_back(5);
    left = static_cast<Vec&&>(moved);
    if (left.get_allocator().id != 3 || moved.get_allocator().id != 3) {
      return 32;
    }
    if (left.size() != 2 || left[0].value != 4 || left[1].value != 5) {
      return 33;
    }
    if (!moved.empty()) {
      return 34;
    }
    moved.emplace_back(6);
    if (moved.size() != 1 || moved[0].value != 6) {
      return 35;
    }
  }
  if (tracking_bad_deallocate != 0) {
    return 36;
  }
  if (tracking_allocations != tracking_deallocations ||
      tracking_constructs != tracking_destroys) {
    return 37;
  }

  reset_tracking();
  {
    Vec left(Alloc(10));
    left.emplace_back(1);
    Vec right(Alloc(20));
    right.emplace_back(2);
    right.emplace_back(3);

    left.swap(right);
    if (left.get_allocator().id != 20 || right.get_allocator().id != 10) {
      return 40;
    }
    if (left.size() != 2 || left[0].value != 2 || left[1].value != 3) {
      return 41;
    }
    if (right.size() != 1 || right[0].value != 1) {
      return 42;
    }
  }
  if (tracking_bad_deallocate != 0) {
    return 43;
  }
  if (tracking_allocations != tracking_deallocations ||
      tracking_constructs != tracking_destroys) {
    return 44;
  }
  return 0;
}

int main(void) {
  int result = test_non_propagating_assignment();
  if (result != 0) {
    return result;
  }
  result = test_propagating_assignment_and_swap();
  if (result != 0) {
    return result;
  }
  return 0;
}
