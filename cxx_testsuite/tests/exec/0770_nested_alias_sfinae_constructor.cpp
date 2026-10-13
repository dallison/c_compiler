#include <type_traits>

template <bool>
struct KeyArgument {
  template <class Key, class Default>
  using type = Key;
};

template <>
struct KeyArgument<false> {
  template <class Key, class Default>
  using type = Default;
};

struct Key {};

template <class KeyType>
struct Parameters {
  static constexpr bool transparent = true;
  using key_type = KeyType;
};

template <class Params>
struct Tree {
  using params_type = Params;
  using key_type = typename Params::key_type;
};

template <class T>
struct Root {
  using params_type = typename T::params_type;
  using key_type = typename T::key_type;

 protected:
  template <class K>
  using key_arg =
      typename KeyArgument<params_type::transparent>::template type<K,
                                                                    key_type>;

 public:
  template <class K = key_type, class... Args>
  int find(const key_arg<K>&, Args&&...) {
    return 1;
  }
};

template <class T>
struct Set : Root<T> {
  using super = Root<T>;

 protected:
  template <class K>
  using key_arg = typename super::template key_arg<K>;
};

template <class T>
struct Map : Set<T> {
  using super = Set<T>;

 private:
  template <class K>
  using key_arg = typename super::template key_arg<K>;

 public:
  using key_type = typename T::key_type;

  template <class K = key_type, class... Args,
            std::enable_if_t<!std::is_convertible<K, int>::value, int> = 0>
  int put(const key_arg<K>&, Args&&...) {
    return 2;
  }
};

template <class T>
struct PublicMap : Map<T> {
  using Base = typename PublicMap::Map;
  using Base::find;
  using Base::put;
};

int main() {
  PublicMap<Tree<Parameters<Key>>> map;
  Key key;
  return map.put(key, 7) == 2 && map.find(key) == 1 ? 0 : 1;
}
