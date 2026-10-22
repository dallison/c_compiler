// RUN: -std=c++17
// EXPECT_EXIT: 0
// A pack-expanded base may nest another expansion of the same pack.
// absl::container_internal::CompressedTuple inherits
// Storage<Ts, I, StorageTag<Ts...>>...; the tag must stay the full pack.

#include <utility>

template <class... Ts>
struct Tag {};

template <class T, size_t I, class TagT, bool UseBase = false>
struct Storage {
  int id() { return (int)I; }
};

template <class Seq, class... Ts>
struct Impl;

template <size_t... I, class... Ts>
struct Impl<std::index_sequence<I...>, Ts...> : Storage<Ts, I, Tag<Ts...>>... {};

template <class A, class B>
struct Tuple : Impl<std::index_sequence<0, 1>, A, B> {
  int cf_id() {
    return static_cast<Storage<A, 0, Tag<A, B>>*>(this)->id();
  }
};

struct CF {};
struct H {};

int main() {
  Tuple<CF, H> value;
  Storage<CF, 0, Tag<CF, H>>* from_tuple = &value;
  Impl<std::index_sequence<0, 1>, CF, H>* impl = &value;
  Storage<CF, 0, Tag<CF, H>>* from_impl = impl;
  Storage<H, 1, Tag<CF, H>>* second = &value;
  if (from_tuple->id() != 0 || from_impl->id() != 0 || second->id() != 1) {
    return 1;
  }
  if (value.cf_id() != 0) {
    return 2;
  }
  return 0;
}
