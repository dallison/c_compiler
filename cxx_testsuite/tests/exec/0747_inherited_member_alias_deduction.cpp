// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class...>
using VoidT = void;

template <class T, class = void>
struct IsTransparent : std::false_type {};

template <class T>
struct IsTransparent<T, VoidT<typename T::is_transparent>>
    : std::true_type {};

template <bool>
struct KeyArg {
  template <class K, class Key>
  using type = K;
};

template <>
struct KeyArg<false> {
  template <class K, class Key>
  using type = Key;
};

struct Transparent {
  using is_transparent = void;
};

template <class Key, class Value>
struct Policy {
  using key_type = Key;
  using mapped_type = Value;
};

template <class PolicyT, class Hash, class Equal, class Alloc>
struct RawSet {
  typename PolicyT::mapped_type storage = 0;
};

template <class PolicyT, class Hash, class Equal, class Alloc>
struct RawMap : RawSet<PolicyT, Hash, Equal, Alloc> {
  template <class K>
  using key_arg =
      typename KeyArg<IsTransparent<Equal>::value &&
                      IsTransparent<Hash>::value>::template type<
          K, typename PolicyT::key_type>;

  using key_type = typename PolicyT::key_type;
  using mapped_type = typename PolicyT::mapped_type;

  template <class K = key_type, class P = PolicyT, int = 0>
  mapped_type& operator[](const key_arg<K>& key) {
    this->storage = static_cast<mapped_type>(sizeof(key));
    return this->storage;
  }
};

template <class Key, class Value, class Hash = Transparent,
          class Equal = Transparent, class Alloc = int>
struct Map : RawMap<Policy<Key, Value>, Hash, Equal, Alloc> {
  using Base = RawMap<Policy<Key, Value>, Hash, Equal, Alloc>;
  using Base::operator[];
};

struct Other {
  char bytes[7];
};

int main() {
  Map<int, int> map;
  Other other{};
  return map[other] == 7 ? 0 : 1;
}
