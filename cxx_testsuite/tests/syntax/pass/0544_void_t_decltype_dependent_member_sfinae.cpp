// RUN: -std=c++20
//
// A partial specialization guarded by void_t<decltype(expr)> must SFINAE-fail
// when expr names a missing member on the substituted type, rather than
// treating the ill-formed decltype as int and matching the specialization.

#include <utility>

template <class T>
struct void_helper {
  using type = void;
};

template <class T>
using void_t = typename void_helper<T>::type;

template <class Alloc, class = void>
struct has_select_on_copy {
  static const int value = 0;
};

template <class Alloc>
struct has_select_on_copy<
    Alloc, void_t<decltype(std::declval<const Alloc&>()
                             .select_on_container_copy_construction())>> {
  static const int value = 1;
};

template <class T>
struct allocator {
  using value_type = T;
};

struct with_select {
  with_select select_on_container_copy_construction() const { return *this; }
};

static_assert(has_select_on_copy<allocator<int>>::value == 0,
              "std::allocator-like type lacks select_on_container_copy_construction");
static_assert(has_select_on_copy<with_select>::value == 1,
              "select_on_container_copy_construction member enables specialization");

int main() {
  return has_select_on_copy<allocator<int>>::value;
}
