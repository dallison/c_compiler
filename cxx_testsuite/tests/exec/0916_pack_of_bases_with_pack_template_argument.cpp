// googletest's FlatTuple: the base-specifier pack
// `ElemBase<FlatTuple<T...>, Idx>...` expands only Idx; the inner `T...`
// is a complete pack expansion and must stay whole in every base.
#include <cstddef>
#include <utility>

template <std::size_t> struct Ignore { Ignore(...); };
template <class> struct Impl;
template <std::size_t... I> struct Impl<std::index_sequence<I...>> {
  template <typename R> static R Apply(Ignore<0 * I>..., R (*)(), ...);
};
template <std::size_t N, typename... T> struct ElemFromList {
  using type = decltype(Impl<std::make_index_sequence<N>>::Apply(
      static_cast<T (*)()>(nullptr)...));
};

template <typename... T> class FlatTuple;
template <typename Derived, std::size_t I> struct ElemBase;
template <typename... T, std::size_t I>
struct ElemBase<FlatTuple<T...>, I> {
  using value_type = typename ElemFromList<I, T...>::type;
  value_type value;
};
template <typename Derived, typename Idx> struct FlatTupleBase;
template <std::size_t... Idx, typename... T>
struct FlatTupleBase<FlatTuple<T...>, std::index_sequence<Idx...>>
    : ElemBase<FlatTuple<T...>, Idx>... {};
template <typename... T>
class FlatTuple
    : public FlatTupleBase<FlatTuple<T...>,
                           std::make_index_sequence<sizeof...(T)>> {};

struct Big { char b[40]; };
using Tuple = FlatTuple<char, Big, double>;

static_assert(sizeof(ElemBase<Tuple, 0>::value_type) == 1, "");
static_assert(sizeof(ElemBase<Tuple, 1>::value_type) == 40, "");
static_assert(sizeof(ElemBase<Tuple, 2>::value_type) == 8, "");

int main() {
  Tuple t;
  static_cast<ElemBase<Tuple, 0>&>(t).value = 'x';
  static_cast<ElemBase<Tuple, 1>&>(t).value.b[39] = 5;
  static_cast<ElemBase<Tuple, 2>&>(t).value = 2.5;
  int failures = 0;
  if (sizeof(t) < 1 + 40 + 8) failures |= 1;
  const Tuple& c = t;
  if (static_cast<const ElemBase<Tuple, 0>&>(c).value != 'x') failures |= 2;
  if (static_cast<const ElemBase<Tuple, 1>&>(c).value.b[39] != 5) failures |= 4;
  if (static_cast<const ElemBase<Tuple, 2>&>(c).value != 2.5) failures |= 8;
  FlatTuple<bool> one;
  static_cast<ElemBase<FlatTuple<bool>, 0>&>(one).value = true;
  if (!static_cast<ElemBase<FlatTuple<bool>, 0>&>(one).value) failures |= 16;
  return failures;
}
