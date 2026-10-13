// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>
#include <utility>

template <class Cond>
using EnableIf = std::enable_if_t<Cond::value, int>;

template <bool Value, class T>
using HasValue = std::conditional_t<Value, T, std::negation<T>>;

template <class, class>
struct IsLifetimeBoundAssignment : std::false_type {};

template <bool>
struct KeyArg {
  template <class K, class>
  using type = K;
};

template <class Key>
struct Tree {
  using key_type = Key;
};

template <class T>
struct SetBase {
 protected:
  template <class K>
  using key_arg = typename KeyArg<true>::template type<K, typename T::key_type>;
};

template <class T>
struct MapBase : SetBase<T> {
 private:
  using super_type = SetBase<T>;

  template <class K>
  using key_arg = typename super_type::template key_arg<K>;

  template <class K, bool Value, class = void>
  using LifetimeBoundK =
      HasValue<Value,
               IsLifetimeBoundAssignment<typename T::key_type, K>>;

 public:
  using key_type = typename T::key_type;

  template <class K = key_type,
            int = EnableIf<LifetimeBoundK<K, false, K>>(), class... Args,
            std::enable_if_t<!std::is_convertible<K, int>::value, int> = 0>
  int try_emplace(const key_arg<K>&, Args&&...) {
    return 17;
  }

  template <class K = key_type, class... Args,
            decltype(EnableIf<LifetimeBoundK<K, true, K>>()) = 0,
            std::enable_if_t<!std::is_convertible<K, int>::value, int> = 0>
  int try_emplace(const key_arg<K>&, Args&&...) {
    return 18;
  }

  template <class K = key_type,
            int = EnableIf<LifetimeBoundK<K, false, K*>>(), class... Args,
            std::enable_if_t<!std::is_convertible<K, int>::value, int> = 0>
  int try_emplace(key_arg<K>&&, Args&&...) {
    return 19;
  }

  template <class K = key_type, class... Args,
            decltype(EnableIf<LifetimeBoundK<K, true, K*>>()) = 0,
            std::enable_if_t<!std::is_convertible<K, int>::value, int> = 0>
  int try_emplace(key_arg<K>&&, Args&&...) {
    return 20;
  }
};

struct Key {};

int main() {
  MapBase<Tree<Key>> map;
  Key key;
  return map.try_emplace(key, 1) == 17 ? 0 : 1;
}
