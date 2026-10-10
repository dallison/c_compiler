// RUN: -std=c++20
// An unparenthesized class member access in decltype names the member's
// declared type, even when the object is constexpr and the access could be
// folded to a constant.
template <class A, class B> struct same { static constexpr bool value = false; };
template <class A> struct same<A, A> { static constexpr bool value = true; };

struct K {
  int d;
  const int c;
  unsigned u : 4;
  long l;
};

constexpr K k{1, 2, 3, 4};
const K ck{1, 2, 3, 4};
constexpr const K* pk = &k;

static_assert(same<decltype(k.d), int>::value);
static_assert(same<decltype(k.c), const int>::value);
static_assert(same<decltype(k.u), unsigned>::value);
static_assert(same<decltype(k.l), long>::value);
static_assert(same<decltype(pk->d), int>::value);
static_assert(same<decltype(ck.d), int>::value);
static_assert(same<decltype((k.d)), const int&>::value);
static_assert(same<decltype(k.d + 0), int>::value);
static_assert(k.d == 1 && pk->l == 4);

template <class T> constexpr bool DeclaredInt(const T& t) {
  return same<decltype(t.d), int>::value;
}
static_assert(DeclaredInt(k));

constexpr decltype(auto) member_auto() { return (k.d); }
static_assert(same<decltype(member_auto()), const int&>::value);

int main() { return 0; }
