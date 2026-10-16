// Explicit template arguments of a member function template call that contain
// a pack expansion are expanded when the enclosing template is instantiated,
// with the remaining parameters still deduced from the call arguments.
#include <stddef.h>
#include <stdio.h>
#include <tuple>
#include <utility>

struct Impl {
  template <typename R, typename A0, typename A1>
  R Perform(const A0& a0, const A1& a1) const {
    return (R)a0 * 2 + (R)sizeof(A1) + (R)a1 * 0;
  }
  template <typename R, typename... A>
  R Count(A...) const {
    return (R)sizeof...(A);
  }
};

template <typename R, typename A0, typename A1>
R FreePerform(const A0& a0, const A1&) {
  return (R)a0 * 3;
}

template <class... T>
struct User : Impl {
  using Tup = std::tuple<T...>;
  template <size_t... I>
  int Member(std::index_sequence<I...>) const {
    return static_cast<const Impl&>(*this)
        .template Perform<int, typename std::tuple_element<I, Tup>::type...>(
            5, 'x');
  }
  template <size_t... I>
  int Free(std::index_sequence<I...>) const {
    return FreePerform<int, typename std::tuple_element<I, Tup>::type...>(5,
                                                                          'x');
  }
  template <class... U>
  int Plain() const {
    return this->template Perform<int, U...>(5, 'x');
  }
  template <class... U>
  int Both() const {
    return this->template Perform<int, U...>(7, 'y');
  }
  int Variadic() const { return this->template Count<int, T...>(T()..., 1.5); }
};

int main() {
  User<int> u;
  int a = u.Free(std::index_sequence<0>{});
  int b = u.Plain<int>();
  int c = u.Member(std::index_sequence<0>{});
  int d = u.Both<long, short>();
  User<int, char> v;
  int e = v.Member(std::index_sequence<0, 1>{});
  int f = v.Variadic();
  if (a != 15 || b != 11 || c != 11 || d != 16 || e != 11 || f != 3) {
    printf("%d %d %d %d %d %d\n", a, b, c, d, e, f);
    return 1;
  }
  printf("ok\n");
  return 0;
}
