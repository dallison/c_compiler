// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <map>
#include <type_traits>
#include <utility>
#include <vector>

struct MapCount {
  int allocs;
  int frees;
};

template <class T>
struct MapTrack {
  using value_type = T;
  using propagate_on_container_move_assignment = std::false_type;
  using is_always_equal = std::false_type;
  MapCount* count;
  int id;
  MapTrack(MapCount* c, int i) : count(c), id(i) {}
  template <class U>
  MapTrack(const MapTrack<U>& other) : count(other.count), id(other.id) {}
  T* allocate(std::size_t n) {
    count->allocs += static_cast<int>(n);
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, std::size_t n) {
    count->frees += static_cast<int>(n);
    ::operator delete(p);
  }
  bool operator==(const MapTrack& other) const {
    return id == other.id && count == other.count;
  }
};

static int fail_vector_bool() {
  std::vector<bool> bits;
  for (int i = 0; i < 70; ++i) {
    bits.push_back((i % 3) == 0);
  }
  if (bits.size() != 70 || !bits[0] || bits[1] || !bits[63] || bits[64]) {
    return 1;
  }
  bits.flip();
  if (bits[0] || !bits[1] || bits.front()) {
    return 2;
  }
  bits[1] = false;
  if (bits[1]) {
    return 3;
  }
  std::vector<bool>::reference proxy = bits[2];
  proxy.flip();
  if (bits[2] || bool(proxy)) {
    return 4;
  }

  std::vector<bool> values{true, false, true, false};
  values.insert(values.begin() + 1, false);
  if (values.size() != 5 || values[0] != true || values[1] != false ||
      values[2] != false || values[4] != false) {
    return 5;
  }
  values.erase(values.begin() + 2);
  if (values.size() != 4 || values[2] != true) {
    return 6;
  }
  values.resize(2);
  if (values.size() != 2 || values[1] != false) {
    return 7;
  }
  if (std::erase(values, false) != 1 || values.size() != 1 || !values[0]) {
    return 8;
  }

  std::vector<bool> copy = bits;
  if (copy.size() != bits.size() || copy[10] != bits[10]) {
    return 9;
  }
  std::vector<bool> moved = std::move(copy);
  if (!copy.empty() || moved.size() != bits.size()) {
    return 10;
  }
  return 0;
}

static int fail_multimap() {
  std::multimap<int, int> values;
  values.insert(std::pair<const int, int>(2, 20));
  values.insert(std::pair<const int, int>(1, 10));
  values.insert(std::pair<const int, int>(2, 21));
  values.insert(std::pair<const int, int>(2, 22));
  values.emplace(0, 1);
  if (values.size() != 5 || values.count(2) != 3 || values.count(3) != 0) {
    return 20;
  }
  int keys[] = {0, 1, 2, 2, 2};
  int mapped[] = {1, 10, 20, 21, 22};
  int index = 0;
  for (std::multimap<int, int>::iterator it = values.begin(); it != values.end();
       ++it, ++index) {
    if (it->first != keys[index] || it->second != mapped[index]) {
      return 21;
    }
  }
  std::pair<std::multimap<int, int>::iterator, std::multimap<int, int>::iterator>
      range = values.equal_range(2);
  if (range.first->second != 20 || range.second != values.end()) {
    return 22;
  }
  if (values.erase(2) != 3 || values.size() != 2 || values.find(2) != values.end()) {
    return 23;
  }
  if (values.begin()->first != 0 || values.lower_bound(1)->second != 10) {
    return 24;
  }

  MapCount bucket;
  bucket.allocs = 0;
  bucket.frees = 0;
  {
    using Item = std::pair<const int, int>;
    std::multimap<int, int, std::less<int>, MapTrack<Item>> owned(
        MapTrack<Item>(&bucket, 1));
    owned.insert(std::pair<const int, int>(4, 40));
    owned.insert(std::pair<const int, int>(4, 41));
    if (owned.get_allocator().id != 1 || owned.count(4) != 2) {
      return 25;
    }
  }
  if (bucket.allocs == 0 || bucket.allocs != bucket.frees) {
    return 26;
  }
  return 0;
}

int main() {
  int result = fail_vector_bool();
  if (result != 0) {
    return result;
  }
  return fail_multimap();
}
