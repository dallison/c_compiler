// RUN: -std=c++17
// EXPECT_EXIT: 0

// Naming a class template specialization while one argument is still
// incomplete defers its member bodies, so its member templates are later
// instantiated straight from the class pattern.  Explicit template arguments
// that are pack elements (`get<I>(t)...`) must still bind the member's own
// packs, in mem-initializers, call arguments, and fold expressions.

#include <cstddef>
#include <tuple>
#include <utility>

struct Record;
using Entry = std::pair<const int, Record>;

template <class T>
struct Walker {
  T* unused = nullptr;

  template <class... A, std::size_t... I>
  static int Sum(std::tuple<A...>& t, std::index_sequence<I...>) {
    return (0 + ... + static_cast<int>(std::get<I>(t)));
  }

  template <class... A, std::size_t... I>
  static int Join(std::tuple<A...>& t, std::index_sequence<I...>) {
    return Combine(std::get<I>(t)...);
  }

  static int Combine(int a, int b) { return a * 10 + b; }
};

using RecordWalker = Walker<Record>;

struct Record {
  int value = 0;
  Record(int v, int scale) : value(v * scale) {}
};

int main() {
  int key = 3;
  Entry entry(std::piecewise_construct, std::forward_as_tuple(key),
              std::forward_as_tuple(4, 5));
  if (entry.first != 3 || entry.second.value != 20) {
    return 1;
  }
  std::tuple<int, int> t(3, 4);
  if (RecordWalker::Sum(t, std::index_sequence<0, 1>{}) != 7) {
    return 2;
  }
  if (RecordWalker::Join(t, std::index_sequence<0, 1>{}) != 34) {
    return 3;
  }
  return 0;
}
