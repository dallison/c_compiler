#include <type_traits>

template <class Key, class Value>
struct Layout {
  struct Pair {
    Key key;
    Value value;
  };

  template <class T>
  static constexpr bool compatible() {
    return std::is_standard_layout<T>() && sizeof(T) == sizeof(Pair);
  }

  static constexpr bool value = compatible<Pair>();
};

int main() {
  return !Layout<int, int>::value;
}
