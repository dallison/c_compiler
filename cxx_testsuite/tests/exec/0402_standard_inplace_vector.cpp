// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <array>
#include <inplace_vector>
#include <new>
#include <optional>
#include <utility>

constexpr bool constexpr_operations() {
  std::inplace_vector<int, 8> values{1, 2, 4};
  values.insert(values.begin() + 2, 3);
  values.emplace_back(5);
  values.resize(7, 9);
  values.erase(values.begin() + 5);
  values.pop_back();
  values.push_back(6);
  if (values.size() != 6 || values.front() != 1 || values.back() != 6) {
    return false;
  }
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (values[i] != static_cast<int>(i + 1)) {
      return false;
    }
  }

  std::array<int, 3> replacement{7, 8, 9};
  values.assign_range(replacement);
  values.append_range(std::array<int, 2>{10, 11});
  return values == std::inplace_vector<int, 8>{7, 8, 9, 10, 11};
}

constexpr bool constexpr_capacity_failure() {
  std::inplace_vector<int, 1> values;
  values.push_back(1);
  try {
    values.push_back(2);
  } catch (const std::bad_alloc&) {
    return values.size() == 1 && values[0] == 1;
  }
  return false;
}

constexpr bool constexpr_push_reference() {
  std::inplace_vector<int, 2> values;
  int& added = values.push_back(7);
  return added == 7;
}

static_assert(constexpr_operations());
static_assert(constexpr_capacity_failure());
static_assert(constexpr_push_reference(), "push reference");

struct tracked {
  int value;
  int* live;

  tracked(int v, int* count) : value(v), live(count) { ++*live; }
  tracked(const tracked& other) : value(other.value), live(other.live) {
    ++*live;
  }
  tracked(tracked&& other) noexcept : value(other.value), live(other.live) {
    ++*live;
    other.value = -1;
  }
  tracked& operator=(const tracked& other) {
    value = other.value;
    return *this;
  }
  tracked& operator=(tracked&& other) noexcept {
    value = other.value;
    other.value = -1;
    return *this;
  }
  ~tracked() { --*live; }
  bool operator==(const tracked& other) const { return value == other.value; }
};

static_assert(!std::is_trivially_copy_constructible_v<tracked>);
static_assert(!std::is_trivially_destructible_v<tracked>);

int main() {
  std::inplace_vector<int, 2> returned_reference;
  int& pushed = returned_reference.push_back(7);
  if (&pushed != &returned_reference.back() || pushed != 7) return 26;

  std::inplace_vector<int, 10> values{1, 4};
  values.insert(values.begin() + 1, {2, 3});
  if (values.size() != 4) return 21;
  if (values[0] != 1) return 22;
  if (values[1] != 2) return 23;
  if (values[2] != 3) return 24;
  if (values[3] != 4) return 25;

  int source[] = {5, 6};
  values.insert(values.end(), source, source + 2);
  if (values.back() != 6 || values.size() != 6) return 2;

  auto inserted = values.try_push_back(7);
  if (!inserted || &*inserted != &values.back() || *inserted != 7) return 3;
  values.unchecked_push_back(8);
  values.unchecked_emplace_back(9);
  values.emplace_back(10);
  if (values.try_emplace_back(11) != std::nullopt) return 4;

  bool capacity_threw = false;
  try {
    values.reserve(11);
  } catch (const std::bad_alloc&) {
    capacity_threw = true;
  }
  if (!capacity_threw || values.size() != 10) return 5;

  if (std::erase_if(values, [](int value) { return value % 2 == 0; }) != 5)
    return 6;
  if (std::erase(values, 7) != 1) return 7;
  if (values != std::inplace_vector<int, 10>{1, 3, 5, 9}) return 8;

  std::inplace_vector<int, 10> other{20, 21};
  int* values_data = values.data();
  int* other_data = other.data();
  values.swap(other);
  if (values.data() != values_data || other.data() != other_data) return 9;
  if (values != std::inplace_vector<int, 10>{20, 21}) return 10;
  if (other != std::inplace_vector<int, 10>{1, 3, 5, 9}) return 11;

  int live = 0;
  {
    std::inplace_vector<tracked, 5> objects;
    objects.emplace_back(1, &live);
    objects.emplace_back(3, &live);
    objects.emplace(objects.begin() + 1, 2, &live);
    if (live != 3 || objects[1].value != 2) return 12;
    std::inplace_vector<tracked, 5> copied(objects);
    if (live != 6 || copied.size() != 3) return 13;
    copied.erase(copied.begin());
    if (live != 5 || copied.front().value != 2) return 14;
    copied.clear();
    if (live != 3) return 15;
  }
  if (live != 0) return 16;

  std::inplace_vector<int, 0> empty;
  if (!empty.empty() || empty.data() != nullptr || empty.capacity() != 0)
    return 17;
  if (empty.try_push_back(1) != std::nullopt) return 18;
  bool empty_threw = false;
  try {
    empty.push_back(1);
  } catch (const std::bad_alloc&) {
    empty_threw = true;
  }
  if (!empty_threw) return 19;

  return 0;
}
