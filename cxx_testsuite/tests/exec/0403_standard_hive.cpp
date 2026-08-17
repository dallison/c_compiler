// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <array>
#include <hive>
#include <memory>
#include <ranges>
#include <utility>

struct tracked {
  static int alive;
  static int constructions;
  int value;

  tracked(int v = 0) : value(v) {
    ++alive;
    ++constructions;
  }
  tracked(const tracked& other) : value(other.value) {
    ++alive;
    ++constructions;
  }
  tracked(tracked&& other) : value(other.value) {
    other.value = -1;
    ++alive;
    ++constructions;
  }
  tracked& operator=(const tracked& other) {
    value = other.value;
    return *this;
  }
  tracked& operator=(tracked&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
  ~tracked() { --alive; }
  bool operator<(const tracked& other) const { return value < other.value; }
  bool operator==(const tracked& other) const { return value == other.value; }
};

int tracked::alive = 0;
int tracked::constructions = 0;

struct construction_error {};

struct throwing_value {
  static bool fail;
  int value;

  explicit throwing_value(int v) : value(v) {
    if (fail) {
      throw construction_error();
    }
  }
};

bool throwing_value::fail = false;

static int allocations = 0;
static int deallocations = 0;

static bool erase_large_value(int value) {
  return value == 5;
}

template <class T>
struct counting_allocator {
  using value_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;

  template <class U>
  struct rebind {
    using other = counting_allocator<U>;
  };

  counting_allocator() = default;
  template <class U>
  counting_allocator(const counting_allocator<U>&) {}

  T* allocate(size_t count) {
    ++allocations;
    return std::allocator<T>().allocate(count);
  }
  void deallocate(T* pointer, size_t count) {
    ++deallocations;
    std::allocator<T>().deallocate(pointer, count);
  }
  size_type max_size() const {
    return static_cast<size_type>(-1) / sizeof(T);
  }
  template <class U, class... Args>
  void construct(U* pointer, Args&&... args) {
    std::allocator<U>().construct(pointer, std::forward<Args>(args)...);
  }
  template <class U>
  void destroy(U* pointer) {
    std::allocator<U>().destroy(pointer);
  }

  template <class U>
  bool operator==(const counting_allocator<U>&) const {
    return true;
  }
};

static int stable_storage_and_holes() {
  std::hive<int> values(std::hive_limits(2, 4));
  for (int i = 0; i < 20; ++i) {
    values.emplace(i);
  }
  int* first_address = &*values.begin();
  std::hive<int>::iterator saved = values.begin() + 10;
  int* saved_address = &*saved;
  if (*saved != 10 || values.get_iterator(saved_address) != saved) return 1;

  for (int i = 20; i < 80; ++i) {
    values.emplace(i);
  }
  if (&*values.begin() != first_address || &*saved != saved_address ||
      *saved != 10) {
    return 2;
  }

  std::hive<int>::iterator hole = values.begin() + 2;
  int* hole_address = &*hole;
  values.erase(hole);
  values.erase(values.begin() + 2);
  values.erase(values.begin() + 2);
  std::hive<int>::iterator reused = values.emplace(1000);
  if (&*reused != hole_address || *reused != 1000) return 3;

  int count = 0;
  int reverse_count = 0;
  for (int value : values) {
    (void)value;
    ++count;
  }
  for (auto iterator = values.rbegin(); iterator != values.rend(); ++iterator) {
    ++reverse_count;
  }
  if (count != 78 || reverse_count != count || values.size() != 78) return 4;
  return 0;
}

static int lifetime_and_rollback() {
  tracked::alive = 0;
  tracked::constructions = 0;
  {
    std::hive<tracked> values(std::hive_limits(2, 8));
    values.emplace(1);
    values.emplace(2);
    values.emplace(3);
    if (tracked::alive != 3) return 10;
    values.erase(values.begin() + 1);
    if (tracked::alive != 2) return 11;
    values.clear();
    if (tracked::alive != 0) return 12;
  }
  if (tracked::alive != 0) return 13;

  std::hive<throwing_value> values(std::hive_limits(2, 4));
  values.emplace(1);
  throwing_value::fail = true;
  try {
    values.emplace(2);
    return 14;
  } catch (const construction_error&) {
  }
  throwing_value::fail = false;
  if (values.size() != 1) return 15;
  values.emplace(3);
  if (values.size() != 2) return 16;
  return 0;
}

static int capacity_limits_and_copy() {
  std::hive<int> values(std::hive_limits(3, 6));
  values.reserve(25);
  if (values.capacity() < 25 || values.size() != 0) return 20;
  size_t reserved_capacity = values.capacity();
  for (int i = 0; i < 9; ++i) values.emplace(i);
  if (values.capacity() != reserved_capacity) return 21;
  values.trim_capacity(values.size());
  if (values.capacity() < values.size()) return 22;
  values.reshape(std::hive_limits(2, 4));
  if (values.size() != 9 || values.block_capacity_limits().min != 2 ||
      values.block_capacity_limits().max != 4) {
    return 23;
  }

  std::hive<int> copied(values);
  if (copied.block_capacity_limits().min != 2 ||
      copied.block_capacity_limits().max != 4 || copied.size() != 9) {
    return 24;
  }
  copied.reserve(30);
  copied.shrink_to_fit();
  if (copied.capacity() > 30 || copied.capacity() < copied.size()) return 25;
  copied.trim_capacity();
  if (copied.capacity() < copied.size()) return 26;
  return 0;
}

static int allocator_ownership() {
  allocations = 0;
  deallocations = 0;
  {
    std::hive<int, counting_allocator<int>> values(std::hive_limits(2, 4));
    values.reserve(12);
    values.insert(8, 7);
    values.erase(values.begin(), values.begin() + 3);
    if (allocations == 0 || deallocations > allocations) return 27;
  }
  if (allocations != deallocations) return 28;
  return 0;
}

static int nonbinding_shrink_preserves_values() {
  std::hive<tracked> left(std::hive_limits(2, 8));
  std::hive<tracked> right(std::hive_limits(2, 8));
  left.emplace(1);
  right.emplace(2);
  right.emplace(3);
  left.splice(right);
  size_t capacity = left.capacity();
  left.shrink_to_fit();
  if (left.capacity() > capacity || left.size() != 3) return 29;
  int expected = 1;
  for (const tracked& value : left) {
    if (value.value != expected++) return 30;
  }
  return 0;
}

static int splice_sort_unique_and_ranges() {
  std::hive<int> left(std::hive_limits(2, 8));
  std::hive<int> right(std::hive_limits(2, 8));
  left.insert({4, 2});
  right.insert({3, 3, 1});
  std::hive<int>::iterator transferred = right.begin();
  int* transferred_address = &*transferred;
  left.splice(right);
  if (!right.empty() || left.size() != 5 || &*transferred != transferred_address ||
      *transferred != 3) {
    return 30;
  }

  left.sort();
  int expected_sorted[] = {1, 2, 3, 3, 4};
  int index = 0;
  for (int value : left) {
    if (value != expected_sorted[index++]) return 31;
  }
  if (left.unique() != 1 || left.size() != 4) return 32;

  std::array<int, 3> more{5, 6, 7};
  left.insert_range(more);
  if (left.size() != 7) return 33;
  std::hive<int> ranged(std::from_range, more);
  if (ranged.size() != 3) return 34;
  ranged.assign_range(more);
  if (ranged.size() != 3) return 35;
  if (std::erase(left, 6) != 1) return 36;
  if (std::erase_if(left, erase_large_value) != 1) return 37;
  return 0;
}

int main() {
  int result = stable_storage_and_holes();
  if (result != 0) return result;
  result = lifetime_and_rollback();
  if (result != 0) return result;
  result = capacity_limits_and_copy();
  if (result != 0) return result;
  result = allocator_ownership();
  if (result != 0) return result;
  result = nonbinding_shrink_preserves_values();
  if (result != 0) return result;
  return splice_sort_unique_and_ranges();
}
