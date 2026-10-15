// googletest's ElemFromList: select the I-th type of a pack by overload
// resolution against `Apply(Ignore<0 * I>..., R (*)(), ...)`.  This needs
//  - a function parameter pack whose pattern is an expression argument,
//  - implicit conversion through a C-variadic constructor `Ignore(...)`,
//  - a qualified alias template with a pack (`seq::index_sequence<>`) inside
//    a class template selecting the partial specialization, and
//  - a cast to a pack type expanded inside decltype.
// An unresolved type can compare equal to anything, so the sizeof checks
// against a 24-byte class are the ones that cannot pass by accident.

template <class T, class U>
struct is_same {
  static constexpr bool value = false;
};
template <class T>
struct is_same<T, T> {
  static constexpr bool value = true;
};
template <class T, class U>
constexpr bool exactly = is_same<T, U>::value && !is_same<T, void>::value;

namespace seq {
template <class T, T... Ints>
struct integer_sequence {};
template <unsigned long... Ints>
using index_sequence = integer_sequence<unsigned long, Ints...>;
template <unsigned long N, unsigned long... Ints>
struct make_impl : make_impl<N - 1, N - 1, Ints...> {};
template <unsigned long... Ints>
struct make_impl<0, Ints...> {
  using type = index_sequence<Ints...>;
};
template <unsigned long N>
using make_index_sequence = typename make_impl<N>::type;
}  // namespace seq

int ignored = 0;
template <unsigned long>
struct Ignore {
  Ignore(...) { ++ignored; }
};

template <typename>
struct ElemFromListImpl;
template <unsigned long... I>
struct ElemFromListImpl<seq::index_sequence<I...>> {
  template <typename R>
  static R Apply(Ignore<0 * I>..., R (*)(), ...);
  static int Count(Ignore<I + 1>..., int last) { return last; }
};

template <unsigned long N, typename... T>
struct ElemFromList {
  using type = decltype(ElemFromListImpl<seq::make_index_sequence<N>>::Apply(
      static_cast<T (*)()>(nullptr)...));
};

template <class F>
struct Function;
template <class R, class... Args>
struct Function<R(Args...)> {
  using Result = R;
  template <unsigned long I>
  using Arg = ElemFromList<I, Args...>;
};

template <class T>
struct Holder {
  using Empty = decltype(ElemFromListImpl<seq::index_sequence<>>::Apply(
      static_cast<T (*)()>(nullptr)));
};

template <class... T>
struct FirstOf {
  using type = decltype(ElemFromListImpl<seq::index_sequence<>>::Apply(
      static_cast<T (*)()>(nullptr)...));
};

static_assert(exactly<ElemFromList<0, char*>::type, char*>, "first of one");
static_assert(exactly<ElemFromList<1, int, double, char>::type, double>, "middle");
static_assert(exactly<ElemFromList<2, int, double, char>::type, char>, "last");
static_assert(exactly<Function<void(char*)>::Arg<0>::type, char*>, "Arg<0>");
static_assert(!is_same<Function<void(char*)>::Arg<0>::type, int>::value, "not int");
static_assert(exactly<Holder<long>::Empty, long>, "qualified alias in template");
static_assert(exactly<FirstOf<short>::type, short>, "cast pack in decltype");

struct Big {
  char bytes[24];
};
static_assert(sizeof(ElemFromList<0, Big>::type) == sizeof(Big), "size first");
static_assert(sizeof(ElemFromList<1, int, Big, char>::type) == sizeof(Big), "size middle");
static_assert(sizeof(Function<void(int, Big)>::Arg<1>::type) == sizeof(Big), "size Arg<1>");
static_assert(sizeof(Holder<Big>::Empty) == sizeof(Big), "size qualified alias");
static_assert(sizeof(FirstOf<Big>::type) == sizeof(Big), "size cast pack");

using F = Function<int(int, double)>;

struct Mock {
  char* seen = nullptr;
  typename Function<void(char*)>::Result Set(typename Function<void(char*)>::template Arg<0>::type p) {
    seen = p;
    return;
  }
  typename F::Result Mix(typename F::template Arg<0>::type a, typename F::template Arg<1>::type b) {
    return a + static_cast<int>(b * 2);
  }
};

int main() {
  int failures = 0;
  Mock m;
  char c = 'x';
  m.Set(&c);
  if (m.seen != &c) failures |= 1;
  if (m.Mix(3, 2.5) != 8) failures |= 2;
  if (ElemFromListImpl<seq::index_sequence<0, 1>>::Count(nullptr, 5, 7) != 7) failures |= 4;
  if (ignored != 2) failures |= 8;
  // Repeated at run time: a static_assert on an unresolved type's size has
  // been silently skipped before.
  unsigned long sizes[] = {sizeof(ElemFromList<0, Big>::type),
                           sizeof(ElemFromList<1, int, Big, char>::type),
                           sizeof(Function<void(int, Big)>::Arg<1>::type),
                           sizeof(Holder<Big>::Empty), sizeof(FirstOf<Big>::type)};
  for (unsigned long i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
    if (sizes[i] != sizeof(Big)) failures |= 16 << i;
  }
  return failures;
}
