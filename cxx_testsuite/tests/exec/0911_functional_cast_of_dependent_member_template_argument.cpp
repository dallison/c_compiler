// `bool(P::value)` as a template argument is a functional cast of the
// dependent member `P::value`, not a function type taking a `P::value`
// parameter.  This is the shape of googletest's `conjunction`.

template <bool B>
struct bool_constant {
  static constexpr bool value = B;
};
using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

template <bool B, class T, class F>
struct conditional {
  using type = T;
};
template <class T, class F>
struct conditional<false, T, F> {
  using type = F;
};

template <typename...>
struct conjunction : true_type {};
template <typename P1>
struct conjunction<P1> : P1 {};
template <typename P1, typename... Ps>
struct conjunction<P1, Ps...>
    : conditional<bool(P1::value), conjunction<Ps...>, P1>::type {};

template <typename P>
struct negation : bool_constant<!bool(P::value)> {};

template <typename P>
struct braced : bool_constant<bool{P::value}> {};

static_assert(conjunction<>::value, "empty");
static_assert(conjunction<true_type, true_type>::value, "all true");
static_assert(!conjunction<false_type, true_type>::value, "first false");
static_assert(!conjunction<true_type, false_type>::value, "last false");
static_assert(negation<false_type>::value, "negation");
static_assert(braced<true_type>::value, "braced");

int main() {
  int failures = 0;
  if (!conjunction<true_type, true_type, true_type>::value) failures |= 1;
  if (conjunction<true_type, false_type, true_type>::value) failures |= 2;
  if (negation<true_type>::value) failures |= 4;
  return failures;
}
