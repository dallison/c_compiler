// RUN: -std=c++17
// EXPECT_EXIT: 0

// A static member variable template whose type is a class specialization
// (`std::atomic<uint32_t>`) is a variable template. Marking that class as a
// nested class template makes the next use collide with the existing tag.

#include <atomic>
#include <stdint.h>

struct Msg {};

template <class T>
struct Box {
  template <class U>
  struct Inner {
    static int id() { return (int)sizeof(U); }
  };

  template <class M>
  static int meta() {
    return (int)metadata_hint<M>.load();
  }

  template <class M>
  static void put(uint32_t value) {
    metadata_hint<M>.store(value);
  }

  template <class M>
  static inline std::atomic<uint32_t> metadata_hint{0};
};

struct Other {};

int main() {
  if (Box<int>::meta<Msg>() != 0) {
    return 1;
  }
  Box<int>::put<Msg>(4);
  if (Box<int>::meta<Msg>() != 4) {
    return 2;
  }
  if (Box<int>::meta<Other>() != 0) {
    return 3;
  }
  Box<char>::put<Msg>(9);
  if (Box<int>::meta<Msg>() != 4) {
    return 4;
  }
  if (Box<char>::meta<Msg>() != 9) {
    return 5;
  }
  if (Box<int>::Inner<char>::id() != 1) {
    return 6;
  }
  return 0;
}
