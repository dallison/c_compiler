// RUN: -std=c++20
// EXPECT_EXIT: 0
// Allocator propagation for vector, list, and map, plus std::erase.
#include <list>
#include <map>
#include <utility>
#include <vector>

struct Count {
  int allocs;
  int frees;
};

template <class T>
struct Track {
  using value_type = T;
  using propagate_on_container_move_assignment = std::false_type;
  using is_always_equal = std::false_type;
  Count* count;
  int id;

  Track(Count* c, int i) : count(c), id(i) {}
  template <class U>
  Track(const Track<U>& other) : count(other.count), id(other.id) {}

  T* allocate(std::size_t n) {
    count->allocs += static_cast<int>(n);
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, std::size_t n) {
    count->frees += static_cast<int>(n);
    ::operator delete(p);
  }
  bool operator==(const Track& other) const {
    return id == other.id && count == other.count;
  }
  bool operator!=(const Track& other) const { return !(*this == other); }
};

static int fail_vector() {
  Count same;
  same.allocs = 0;
  same.frees = 0;
  using Vec = std::vector<int, Track<int>>;
  Vec source(Track<int>(&same, 1));
  source.push_back(1);
  source.push_back(2);
  source.push_back(3);
  Vec dest(Track<int>(&same, 1));
  dest = std::move(source);
  if (!source.empty() || dest.size() != 3 || dest[2] != 3) {
    return 1;
  }
  if (std::erase(dest, 2) != 1 || dest.size() != 2 || dest[1] != 3) {
    return 2;
  }

  Count left;
  left.allocs = 0;
  left.frees = 0;
  Count right;
  right.allocs = 0;
  right.frees = 0;
  {
    Vec a(Track<int>(&left, 1));
    a.push_back(7);
    a.push_back(8);
    Vec b(Track<int>(&right, 2));
    b = std::move(a);
    if (b.size() != 2 || b[0] != 7 || b[1] != 8) {
      return 3;
    }
    if (b.get_allocator().id != 2 || a.get_allocator().id != 1) {
      return 4;
    }
  }
  if (left.allocs == 0 || left.allocs != left.frees || right.allocs == 0 ||
      right.allocs != right.frees) {
    return 5;
  }
  return 0;
}

static int fail_list() {
  Count bucket;
  bucket.allocs = 0;
  bucket.frees = 0;
  using Lst = std::list<int, Track<int>>;
  {
    Lst values(Track<int>(&bucket, 4));
    if (values.get_allocator().id != 4) {
      return 10;
    }
    values.push_back(1);
    values.push_back(3);
    values.push_back(2);
    values.sort();
    if (values.front() != 1 || *(++values.begin()) != 2) {
      return 11;
    }
    if (std::erase(values, 2) != 1 || values.size() != 2) {
      return 12;
    }
  }

  Count other;
  other.allocs = 0;
  other.frees = 0;
  {
    Lst src(Track<int>(&bucket, 4));
    src.push_back(9);
    Lst dst(Track<int>(&other, 5));
    dst = std::move(src);
    if (dst.size() != 1 || dst.front() != 9 || dst.get_allocator().id != 5) {
      return 13;
    }
    if (src.get_allocator().id != 4) {
      return 14;
    }
  }
  if (bucket.allocs == 0 || bucket.allocs != bucket.frees || other.allocs == 0 ||
      other.allocs != other.frees) {
    return 15;
  }
  return 0;
}

static int fail_map() {
  Count bucket;
  bucket.allocs = 0;
  bucket.frees = 0;
  using Map = std::map<int, int, std::less<int>, Track<std::pair<const int, int>>>;
  {
    Map values(Track<std::pair<const int, int>>(&bucket, 1));
    values.insert(std::pair<const int, int>(1, 10));
    values.insert(std::pair<const int, int>(2, 20));
    values[3] = 30;
    Map::node_type node = values.extract(2);
    if (node.empty() || node.mapped() != 20 || values.size() != 2) {
      return 20;
    }
  }
  if (bucket.allocs == 0 || bucket.allocs != bucket.frees) {
    return 21;
  }

  Count left;
  left.allocs = 0;
  left.frees = 0;
  Count right;
  right.allocs = 0;
  right.frees = 0;
  {
    Map src(Track<std::pair<const int, int>>(&left, 1));
    src.insert(std::pair<const int, int>(4, 40));
    src.insert(std::pair<const int, int>(5, 50));
    Map dst(Track<std::pair<const int, int>>(&right, 2));
    dst = std::move(src);
    if (!src.empty() || dst.size() != 2 || dst.find(5)->second != 50) {
      return 22;
    }
    if (dst.get_allocator().id != 2) {
      return 23;
    }
  }
  if (left.allocs == 0 || left.allocs != left.frees || right.allocs == 0 ||
      right.allocs != right.frees) {
    return 24;
  }
  return 0;
}

static int fail_range_constructors() {
  int values[] = {4, 5, 6};
  std::list<int> numbers(values, values + 3);
  if (numbers.size() != 3 || numbers.front() != 4 || numbers.back() != 6) {
    return 30;
  }
  std::pair<const int, int> pairs[] = {std::pair<const int, int>(1, 10),
                                       std::pair<const int, int>(2, 20)};
  std::map<int, int> table(pairs, pairs + 2);
  if (table.size() != 2 || table.find(2)->second != 20) {
    return 31;
  }
  std::vector<int> copy(values, values + 3);
  if (std::erase_if(copy, [](int value) { return value == 5; }) != 1 ||
      copy.size() != 2 || copy[1] != 6) {
    return 32;
  }
  return 0;
}

int main() {
  int result = fail_range_constructors();
  if (result != 0) {
    return result;
  }
  result = fail_vector();
  if (result != 0) {
    return result;
  }
  result = fail_list();
  if (result != 0) {
    return result;
  }
  return fail_map();
}
