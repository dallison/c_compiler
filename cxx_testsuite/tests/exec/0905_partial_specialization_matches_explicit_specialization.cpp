// A partial specialization `size<tup<Ts...>>` matches an explicit
// specialization of `tup` (`tuple<>` is one in most standard libraries), but
// not one of a same-named template in another namespace.
#include <tuple>

template <class... T> struct tup {};
template <> struct tup<> {};
template <> struct tup<int> { int x; };
template <class A, class B> struct two {};
template <> struct two<int, int> {};

namespace other {
template <class... T> struct tup {};
template <> struct tup<char> {};
}  // namespace other

template <class T> struct size { static constexpr int value = -1; };
template <class... Ts> struct size<tup<Ts...>> {
  static constexpr int value = sizeof...(Ts);
};
template <class A, class B> struct size<two<A, B>> {
  static constexpr int value = 2;
};

namespace legacy {
using std::tuple_size;
}

static_assert(size<tup<>>::value == 0, "");
static_assert(size<tup<int>>::value == 1, "");
static_assert(size<tup<int, char>>::value == 2, "");
static_assert(size<two<int, int>>::value == 2, "");
static_assert(size<other::tup<char>>::value == -1, "");
static_assert(std::tuple_size<std::tuple<>>::value == 0, "");
static_assert(::legacy::tuple_size<std::tuple<>>::value == 0, "");

int main() {
  return size<tup<>>::value + size<tup<int>>::value - 1;
}
