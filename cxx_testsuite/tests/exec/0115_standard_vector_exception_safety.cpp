// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <new>
#include <utility>
#include <vector>

void throw_construct_failure(void) {
  throw 901;
}

int alloc_state_allocations = 0;
int alloc_state_deallocations = 0;
int alloc_state_constructs = 0;
int alloc_state_destroys = 0;
int alloc_state_throw_after_construct = 0;

struct TrackedValue {
  int value;

  static int live;
  static int copied;
  static int moved;

  explicit TrackedValue(int v) : value(v) {
    ++live;
  }

  TrackedValue(const TrackedValue& other) : value(other.value) {
    ++live;
    ++copied;
  }

  TrackedValue(TrackedValue&& other) : value(other.value) {
    other.value = -1;
    ++live;
    ++moved;
  }

  TrackedValue& operator=(const TrackedValue& other) {
    value = other.value;
    return *this;
  }

  TrackedValue& operator=(TrackedValue&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }

  ~TrackedValue() {
    --live;
  }
};

int TrackedValue::live = 0;
int TrackedValue::copied = 0;
int TrackedValue::moved = 0;

void maybe_throw_construct(void) {
  if (alloc_state_throw_after_construct > 0) {
    --alloc_state_throw_after_construct;
    if (alloc_state_throw_after_construct == 0) {
      throw_construct_failure();
    }
  }
}

template <class T>
struct ThrowingAllocator {
  using value_type = T;
  using size_type = unsigned long;
  using difference_type = long;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  ThrowingAllocator() noexcept {}
  ThrowingAllocator(const ThrowingAllocator&) noexcept {}

  template <class U>
  ThrowingAllocator(const ThrowingAllocator<U>&) noexcept {}

  pointer allocate(size_type n) {
    ++alloc_state_allocations;
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    ++alloc_state_deallocations;
    ::operator delete(static_cast<void*>(ptr));
  }

  void construct(pointer ptr, const T& value) {
    maybe_throw_construct();
    ++alloc_state_constructs;
    new (ptr) T(value);
  }

  void construct(pointer ptr, T&& value) {
    maybe_throw_construct();
    ++alloc_state_constructs;
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    maybe_throw_construct();
    ++alloc_state_constructs;
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    ++alloc_state_destroys;
    ptr->~T();
  }

  size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T, class U>
bool operator==(const ThrowingAllocator<T>&, const ThrowingAllocator<U>&) {
  return true;
}

template <class T, class U>
bool operator!=(const ThrowingAllocator<T>&, const ThrowingAllocator<U>&) {
  return false;
}

void reset_state(void) {
  alloc_state_allocations = 0;
  alloc_state_deallocations = 0;
  alloc_state_constructs = 0;
  alloc_state_destroys = 0;
  alloc_state_throw_after_construct = 0;
  TrackedValue::live = 0;
  TrackedValue::copied = 0;
  TrackedValue::moved = 0;
}

int check_balanced(int code) {
  if (alloc_state_constructs != alloc_state_destroys) {
    return code;
  }
  if (alloc_state_allocations != alloc_state_deallocations) {
    return code + 1;
  }
  return 0;
}

int active_constructed_objects(void) {
  return alloc_state_constructs - alloc_state_destroys;
}

int verify_three(const std::vector<TrackedValue,
                 ThrowingAllocator<TrackedValue> >& values, int code) {
  if (values.size() != 3) {
    return code;
  }
  if (values[0].value != 1 || values[1].value != 2 ||
      values[2].value != 3) {
    return code + 1;
  }
  return 0;
}

int test_reserve_construct_failure(void) {
  reset_state();
  {
    std::vector<TrackedValue, ThrowingAllocator<TrackedValue> > values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    int active_before = active_constructed_objects();
    alloc_state_throw_after_construct = 1;
    try {
      values.reserve(8);
      return 1;
    } catch (...) {
    }
    int result = verify_three(values, 2);
    if (result != 0) {
      return result;
    }
    if (active_constructed_objects() != active_before) {
      return 4;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_balanced(5);
}

int test_emplace_back_construct_failure(void) {
  reset_state();
  {
    std::vector<TrackedValue, ThrowingAllocator<TrackedValue> > values;
    values.reserve(2);
    values.emplace_back(1);
    int active_before = active_constructed_objects();
    alloc_state_throw_after_construct = 1;
    try {
      values.emplace_back(9);
      return 10;
    } catch (...) {
    }
    if (values.size() != 1 || values[0].value != 1) {
      return 11;
    }
    if (active_constructed_objects() != active_before) {
      return 12;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_balanced(13);
}

int test_middle_emplace_inserted_failure(void) {
  reset_state();
  {
    std::vector<TrackedValue, ThrowingAllocator<TrackedValue> > values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    int active_before = active_constructed_objects();
    alloc_state_throw_after_construct = 2;
    try {
      values.emplace(values.begin() + 1, 9);
      return 20;
    } catch (...) {
    }
    int result = verify_three(values, 21);
    if (result != 0) {
      return result;
    }
    if (active_constructed_objects() != active_before) {
      return 23;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_balanced(24);
}

int test_middle_emplace_suffix_failure(void) {
  reset_state();
  {
    std::vector<TrackedValue, ThrowingAllocator<TrackedValue> > values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    int active_before = active_constructed_objects();
    alloc_state_throw_after_construct = 3;
    try {
      values.emplace(values.begin() + 1, 9);
      return 30;
    } catch (...) {
    }
    int result = verify_three(values, 31);
    if (result != 0) {
      return result;
    }
    if (active_constructed_objects() != active_before) {
      return 33;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_balanced(34);
}

int main(void) {
  int result = test_reserve_construct_failure();
  if (result != 0) {
    return result;
  }
  result = test_emplace_back_construct_failure();
  if (result != 0) {
    return result;
  }
  result = test_middle_emplace_inserted_failure();
  if (result != 0) {
    return result;
  }
  result = test_middle_emplace_suffix_failure();
  if (result != 0) {
    return result;
  }
  return 0;
}
