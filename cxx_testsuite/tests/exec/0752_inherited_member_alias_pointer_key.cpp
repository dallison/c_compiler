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

template <class T>
struct HashEq;

template <class T>
struct HashEq<T*> {
  struct Hash {
    using is_transparent = void;
  };
  struct Eq {
    using is_transparent = void;
  };
};

template <class Cond>
using EnableIf = std::enable_if_t<Cond::value, int>;

template <bool Value, class T>
using HasValue = std::conditional_t<Value, T, std::negation<T>>;

template <class To, class From>
struct IsLifetimeBoundAssignment : std::false_type {};

template <class>
struct ElementIsOwner : std::false_type {};

template <class Key, class Value>
struct Policy {
  using key_type = Key;
  using mapped_type = Value;
};

template <class PolicyT, class Hash, class Equal>
struct RawMap {
  template <class P>
  using MappedReference = typename P::mapped_type&;

  template <class K>
  using key_arg =
      typename KeyArg<IsTransparent<Equal>::value &&
                      IsTransparent<Hash>::value>::template type<
          K, typename PolicyT::key_type>;

  template <class K, bool Value, class = void>
  using LifetimeBoundK = HasValue<
      Value,
      std::conditional_t<ElementIsOwner<PolicyT>::value, std::false_type,
                         IsLifetimeBoundAssignment<
                             typename PolicyT::key_type, K>>>;

  using key_type = typename PolicyT::key_type;
  using mapped_type = typename PolicyT::mapped_type;

  mapped_type storage = 0;

  template <class K = key_type, class P = PolicyT,
            int = EnableIf<LifetimeBoundK<K, false, K*>>()>
  MappedReference<P> operator[](key_arg<K>&&) {
    storage = 17;
    return storage;
  }

  template <class K = key_type, class P = PolicyT, int&...,
            EnableIf<LifetimeBoundK<K, true, K*>> = 0>
  MappedReference<P> operator[](key_arg<K>&&) {
    storage = 18;
    return storage;
  }

  template <class K = key_type, class P = PolicyT,
            int = EnableIf<LifetimeBoundK<K, false>>()>
  MappedReference<P> operator[](const key_arg<K>&) {
    storage = 19;
    return storage;
  }

  template <class K = key_type, class P = PolicyT, int&...,
            EnableIf<LifetimeBoundK<K, true>> = 0>
  MappedReference<P> operator[](const key_arg<K>&) {
    storage = 20;
    return storage;
  }
};

template <class Key, class Value,
          class Hash = typename HashEq<Key>::Hash,
          class Equal = typename HashEq<Key>::Eq>
struct Map : RawMap<Policy<Key, Value>, Hash, Equal> {
  using Base = RawMap<Policy<Key, Value>, Hash, Equal>;
  using Base::operator[];
};

struct Item {};

using ItemPolicy = Policy<const Item*, int>;
using ItemRawMap =
    RawMap<ItemPolicy, typename HashEq<const Item*>::Hash,
           typename HashEq<const Item*>::Eq>;
using ItemLifetime =
    ItemRawMap::LifetimeBoundK<const Item*, false, const Item**>;
static_assert(ItemLifetime::value);
static_assert(std::is_same<EnableIf<ItemLifetime>, int>::value);

int main() {
  Map<const Item*, int> map;
  Item item;
  const Item* pointer = &item;
  return map[pointer] == 19 ? 0 : 1;
}
