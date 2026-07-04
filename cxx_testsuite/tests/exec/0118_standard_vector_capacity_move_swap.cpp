// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <new>
#include <utility>
#include <vector>

struct Box {
  int value;

  Box() : value(0) {}
  explicit Box(int v) : value(v) {}
  Box(const Box& other) : value(other.value) {}
  Box(Box&& other) : value(other.value) {
    other.value = -1;
  }
  Box& operator=(const Box& other) {
    value = other.value;
    return *this;
  }
  Box& operator=(Box&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
};

template <class T>
struct IdAllocator {
  using value_type = T;
  using size_type = unsigned long;
  using difference_type = long;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  int id;

  IdAllocator() noexcept : id(0) {}
  explicit IdAllocator(int value) noexcept : id(value) {}
  IdAllocator(const IdAllocator& other) noexcept : id(other.id) {}

  template <class U>
  IdAllocator(const IdAllocator<U>& other) noexcept : id(other.id) {}

  pointer allocate(size_type n) {
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    ::operator delete(static_cast<void*>(ptr));
  }

  void construct(pointer ptr, const T& value) {
    new (ptr) T(value);
  }

  void construct(pointer ptr, T&& value) {
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    ptr->~T();
  }

  size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T, class U>
bool operator==(const IdAllocator<T>& left, const IdAllocator<U>& right) {
  return left.id == right.id;
}

template <class T, class U>
bool operator!=(const IdAllocator<T>& left, const IdAllocator<U>& right) {
  return !(left == right);
}

int test_capacity(void) {
  std::vector<int> values;
  values.reserve(8);
  if (values.capacity() < 8 || values.size() != 0) {
    return 1;
  }

  values.push_back(1);
  values.push_back(2);
  values.push_back(3);
  unsigned long cap = values.capacity();
  values.reserve(2);
  if (values.capacity() != cap || values.size() != 3) {
    return 2;
  }
  if (values[0] != 1 || values[1] != 2 || values[2] != 3) {
    return 3;
  }

  values.shrink_to_fit();
  if (values.capacity() != values.size() || values.capacity() != 3) {
    return 4;
  }
  if (values[0] != 1 || values[2] != 3) {
    return 5;
  }

  values.clear();
  if (!values.empty() || values.size() != 0) {
    return 6;
  }
  values.shrink_to_fit();
  if (values.capacity() != 0 || values.data() != 0) {
    return 7;
  }
  values.push_back(9);
  if (values.size() != 1 || values[0] != 9) {
    return 8;
  }
  return 0;
}

int test_move_and_reuse(void) {
  std::vector<int> source;
  source.push_back(4);
  source.push_back(5);
  source.reserve(6);
  std::vector<int> moved(static_cast<std::vector<int>&&>(source));
  if (moved.size() != 2 || moved[0] != 4 || moved[1] != 5) {
    return 20;
  }
  if (!source.empty() || source.size() != 0 || source.capacity() != 0) {
    return 21;
  }
  source.push_back(6);
  if (source.size() != 1 || source[0] != 6) {
    return 22;
  }

  std::vector<int> assigned;
  assigned.push_back(1);
  assigned = static_cast<std::vector<int>&&>(moved);
  if (assigned.size() != 2 || assigned[0] != 4 || assigned[1] != 5) {
    return 23;
  }
  if (!moved.empty() || moved.size() != 0 || moved.capacity() != 0) {
    return 24;
  }
  moved.push_back(7);
  if (moved.size() != 1 || moved[0] != 7) {
    return 25;
  }
  return 0;
}

int test_swap(void) {
  std::vector<int> left;
  left.reserve(5);
  left.push_back(1);
  left.push_back(2);
  std::vector<int> right;
  right.reserve(3);
  right.push_back(8);
  unsigned long left_cap = left.capacity();
  unsigned long right_cap = right.capacity();
  left.swap(right);
  if (left.size() != 1 || left[0] != 8 || left.capacity() != right_cap) {
    return 30;
  }
  if (right.size() != 2 || right[0] != 1 || right[1] != 2 ||
      right.capacity() != left_cap) {
    return 31;
  }
  left.swap(right);
  if (left.size() != 2 || left[0] != 1 || right.size() != 1 ||
      right[0] != 8) {
    return 32;
  }
  return 0;
}

int test_move_with_unequal_allocator(void) {
  using Vec = std::vector<Box, IdAllocator<Box> >;
  Vec source(IdAllocator<Box>(1));
  source.emplace_back(11);
  source.emplace_back(22);
  Vec moved(static_cast<Vec&&>(source), IdAllocator<Box>(2));
  if (moved.size() != 2 || moved[0].value != 11 || moved[1].value != 22) {
    return 40;
  }
  if (!source.empty() || source.size() != 0) {
    return 41;
  }
  source.emplace_back(33);
  if (source.size() != 1 || source[0].value != 33) {
    return 42;
  }
  if (moved.get_allocator().id != 2 || source.get_allocator().id != 1) {
    return 43;
  }
  return 0;
}

int main(void) {
  int result = test_capacity();
  if (result != 0) {
    return result;
  }
  result = test_move_and_reuse();
  if (result != 0) {
    return result;
  }
  result = test_swap();
  if (result != 0) {
    return result;
  }
  result = test_move_with_unequal_allocator();
  if (result != 0) {
    return result;
  }
  return 0;
}
