// A member function template of a class template whose return type depends
// only on its own parameters: through a decltype written with a cast
// (`decltype(Id((J*)nullptr))`), and through a member alias of another
// template whose pattern expands a pack (googletest's ElemFromList).
#include <cstddef>
#include <type_traits>
#include <utility>

template <class T> T Id(T*);

template <std::size_t> struct Ignore { Ignore(...); };
template <class> struct Impl;
template <std::size_t... I> struct Impl<std::index_sequence<I...>> {
  template <typename R> static R Apply(Ignore<0 * I>..., R (*)(), ...);
};
template <std::size_t N, typename... T> struct ElemFromList {
  using type = decltype(Impl<std::make_index_sequence<N>>::Apply(
      static_cast<T (*)()>(nullptr)...));
};

template <std::size_t N, class T> struct ById {
  using type = decltype(Id((T*)nullptr));
};

struct Big { char b[40]; };

template <class A> struct Q {
  template <class J> decltype(Id((J*)nullptr)) Direct() { return J{}; }
  template <class J> typename ById<1, J>::type ViaAlias() { return J{}; }
  template <std::size_t J> typename ElemFromList<J, char, Big, double>::type
  Pick() {
    return typename ElemFromList<J, char, Big, double>::type{};
  }
};

int main() {
  Q<int> q;
  static_assert(std::is_same<decltype(q.Direct<Big>()), Big>::value, "");
  static_assert(std::is_same<decltype(q.ViaAlias<Big>()), Big>::value, "");
  static_assert(std::is_same<decltype(q.Pick<0>()), char>::value, "");
  static_assert(std::is_same<decltype(q.Pick<1>()), Big>::value, "");
  static_assert(std::is_same<decltype(q.Pick<2>()), double>::value, "");
  Big big = q.Direct<Big>();
  Big via = q.ViaAlias<Big>();
  Big picked = q.Pick<1>();
  int failures = 0;
  if (sizeof(big) != 40 || sizeof(via) != 40 || sizeof(picked) != 40)
    failures |= 1;
  if (sizeof(q.Pick<2>()) != sizeof(double)) failures |= 2;
  if (q.Pick<0>() != 0) failures |= 4;
  return failures;
}
