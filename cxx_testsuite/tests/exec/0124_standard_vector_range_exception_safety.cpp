// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <new>
#include <utility>
#include <vector>

void throw_range_construct_failure(void) {
  throw 902;
}

int range_allocations = 0;
int range_deallocations = 0;
int range_constructs = 0;
int range_destroys = 0;
int range_throw_after_construct = 0;

struct RangeValue {
  int value;

  explicit RangeValue(int v) : value(v) {}
  RangeValue(const RangeValue& other) : value(other.value) {}
  RangeValue(RangeValue&& other) : value(other.value) {
    other.value = -1;
  }
  RangeValue& operator=(const RangeValue& other) {
    value = other.value;
    return *this;
  }
  RangeValue& operator=(RangeValue&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
};

void maybe_throw_range_construct(void) {
  if (range_throw_after_construct > 0) {
    --range_throw_after_construct;
    if (range_throw_after_construct == 0) {
      throw_range_construct_failure();
    }
  }
}

template <class T>
struct RangeThrowingAllocator {
  using value_type = T;
  using size_type = unsigned long;
  using difference_type = long;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  RangeThrowingAllocator() noexcept {}
  RangeThrowingAllocator(const RangeThrowingAllocator&) noexcept {}

  template <class U>
  RangeThrowingAllocator(const RangeThrowingAllocator<U>&) noexcept {}

  pointer allocate(size_type n) {
    ++range_allocations;
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    ++range_deallocations;
    ::operator delete(static_cast<void*>(ptr));
  }

  void construct(pointer ptr, const T& value) {
    maybe_throw_range_construct();
    ++range_constructs;
    new (ptr) T(value);
  }

  void construct(pointer ptr, T&& value) {
    maybe_throw_range_construct();
    ++range_constructs;
    new (ptr) T(static_cast<T&&>(value));
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    maybe_throw_range_construct();
    ++range_constructs;
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    ++range_destroys;
    ptr->~T();
  }

  size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T, class U>
bool operator==(const RangeThrowingAllocator<T>&,
                const RangeThrowingAllocator<U>&) {
  return true;
}

template <class T, class U>
bool operator!=(const RangeThrowingAllocator<T>&,
                const RangeThrowingAllocator<U>&) {
  return false;
}

using RangeVector = std::vector<RangeValue, RangeThrowingAllocator<RangeValue> >;

void reset_range_state(void) {
  range_allocations = 0;
  range_deallocations = 0;
  range_constructs = 0;
  range_destroys = 0;
  range_throw_after_construct = 0;
}

int active_range_objects(void) {
  return range_constructs - range_destroys;
}

int check_range_balanced(int code) {
  if (range_constructs != range_destroys) {
    return code;
  }
  if (range_allocations != range_deallocations) {
    return code + 1;
  }
  return 0;
}

int verify_original_three(const RangeVector& values, int code) {
  if (values.size() != 3) {
    return code;
  }
  if (values[0].value != 1 || values[1].value != 2 ||
      values[2].value != 3) {
    return code + 1;
  }
  return 0;
}

int test_assign_temp_copy_failure(void) {
  reset_range_state();
  {
    RangeVector values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    RangeValue source[3] = {RangeValue(4), RangeValue(5), RangeValue(6)};
    int active_before = active_range_objects();
    range_throw_after_construct = 1;
    try {
      values.assign(source, source + 3);
      return 1;
    } catch (...) {
    }
    int result = verify_original_three(values, 2);
    if (result != 0) {
      return result;
    }
    if (active_range_objects() != active_before) {
      return 4;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_range_balanced(5);
}

int test_assign_temp_growth_move_failure(void) {
  reset_range_state();
  {
    RangeVector values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    RangeValue source[4] = {
        RangeValue(4), RangeValue(5), RangeValue(6), RangeValue(7)};
    int active_before = active_range_objects();
    range_throw_after_construct = 2;
    try {
      values.assign(source, source + 4);
      return 10;
    } catch (...) {
    }
    int result = verify_original_three(values, 11);
    if (result != 0) {
      return result;
    }
    if (active_range_objects() != active_before) {
      return 13;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_range_balanced(14);
}

int test_insert_temp_copy_failure(void) {
  reset_range_state();
  {
    RangeVector values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    RangeValue source[2] = {RangeValue(8), RangeValue(9)};
    int active_before = active_range_objects();
    range_throw_after_construct = 1;
    try {
      values.insert(values.begin() + 1, source, source + 2);
      return 20;
    } catch (...) {
    }
    int result = verify_original_three(values, 21);
    if (result != 0) {
      return result;
    }
    if (active_range_objects() != active_before) {
      return 23;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_range_balanced(24);
}

int test_insert_commit_copy_failure(void) {
  reset_range_state();
  {
    RangeVector values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    RangeValue source[2] = {RangeValue(8), RangeValue(9)};
    int active_before = active_range_objects();
    range_throw_after_construct = 4;
    try {
      values.insert(values.begin() + 1, source, source + 2);
      return 30;
    } catch (...) {
    }
    int result = verify_original_three(values, 31);
    if (result != 0) {
      return result;
    }
    if (active_range_objects() != active_before) {
      return 33;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_range_balanced(34);
}

int test_assign_self_range_temp_failure(void) {
  reset_range_state();
  {
    RangeVector values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    int active_before = active_range_objects();
    range_throw_after_construct = 2;
    try {
      values.assign(values.begin(), values.end());
      return 40;
    } catch (...) {
    }
    int result = verify_original_three(values, 41);
    if (result != 0) {
      return result;
    }
    if (active_range_objects() != active_before) {
      return 43;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_range_balanced(44);
}

int test_insert_self_range_temp_failure(void) {
  reset_range_state();
  {
    RangeVector values;
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(3);
    int active_before = active_range_objects();
    range_throw_after_construct = 2;
    try {
      values.insert(values.begin() + 1, values.begin(), values.end());
      return 50;
    } catch (...) {
    }
    int result = verify_original_three(values, 51);
    if (result != 0) {
      return result;
    }
    if (active_range_objects() != active_before) {
      return 53;
    }
    values.clear();
    values.shrink_to_fit();
  }
  return check_range_balanced(54);
}

int main(void) {
  int result = test_assign_temp_copy_failure();
  if (result != 0) {
    return result;
  }
  result = test_assign_temp_growth_move_failure();
  if (result != 0) {
    return result;
  }
  result = test_insert_temp_copy_failure();
  if (result != 0) {
    return result;
  }
  result = test_insert_commit_copy_failure();
  if (result != 0) {
    return result;
  }
  result = test_assign_self_range_temp_failure();
  if (result != 0) {
    return result;
  }
  result = test_insert_self_range_temp_failure();
  if (result != 0) {
    return result;
  }
  return 0;
}
