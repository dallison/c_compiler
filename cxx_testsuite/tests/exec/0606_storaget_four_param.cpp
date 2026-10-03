// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <stddef.h>
#include <tuple>
#include <type_traits>

template <class T>
constexpr bool ShouldUseBase() {
  return std::is_class<T>::value && std::is_empty<T>::value &&
         !std::is_final<T>::value;
}

template <class T, size_t I, class Tag, bool UseBase = ShouldUseBase<T>()>
struct Storage {
  T value;
  constexpr Storage() : value() {}
  explicit constexpr Storage(T v) : value(v) {}
  constexpr T& get() & { return value; }
};

template <class... Ts>
struct StorageTag;

template <class... Ts>
struct Tuple;

template <class D, size_t I>
struct Elem;
template <class... B, size_t I>
struct Elem<Tuple<B...>, I> : std::tuple_element<I, std::tuple<B...>> {};
template <class D, size_t I>
using ElemT = typename Elem<D, I>::type;

template <class... Ts>
struct Impl : Storage<Ts, 0, StorageTag<Ts...>, false>... {
  Impl(const Ts&... a) : Storage<Ts, 0, StorageTag<Ts...>, false>(a)... {}
};

template <class... Ts>
struct Tuple : Impl<Ts...> {
  Tuple(const Ts&... a) : Impl<Ts...>(a...) {}

  template <int I>
  using ET = ElemT<Tuple, I>;

  template <int I>
  using StorageT = Storage<ET<I>, I, StorageTag<Ts...>>;

  template <int I>
  constexpr ET<I>& get() & {
    return StorageT<I>::get();
  }
};

int main() {
  Tuple<long> t(5l);
  if (t.get<0>() != 5l) {
    return 1;
  }
  Tuple<int> i(7);
  return i.get<0>() == 7 ? 0 : 2;
}
