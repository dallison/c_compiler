// std::pointer_traits for class-type pointers ([pointer.traits]): members come
// from the pointer class when declared, else from its first template argument.
#include <cstddef>
#include <memory>
#include <type_traits>

template <class T, class Tag = void> struct Fancy {
  T* p;
  static Fancy pointer_to(T& r) { return Fancy{&r}; }
};

struct Declares {
  using element_type = long;
  using difference_type = short;
  template <class U> using rebind = U*;
  static Declares pointer_to(long& r) { (void)r; return Declares{}; }
};

template <class T> struct NoArgs {};
template <class T> struct Opaque { T* p; };

template <class P, class = void> struct HasElement : std::false_type {};
template <class P>
struct HasElement<P, std::void_t<typename std::pointer_traits<P>::element_type>>
    : std::true_type {};

using FT = std::pointer_traits<Fancy<int, char>>;
static_assert(std::is_same<FT::pointer, Fancy<int, char>>::value, "");
static_assert(std::is_same<FT::element_type, int>::value, "");
static_assert(std::is_same<FT::difference_type, std::ptrdiff_t>::value, "");
static_assert(std::is_same<FT::rebind<double>, Fancy<double, char>>::value, "");

using DT = std::pointer_traits<Declares>;
static_assert(std::is_same<DT::element_type, long>::value, "");
static_assert(std::is_same<DT::difference_type, short>::value, "");
static_assert(std::is_same<DT::rebind<char>, char*>::value, "");

using UT = std::pointer_traits<std::unique_ptr<int>>;
static_assert(std::is_same<UT::element_type, int>::value, "");
static_assert(std::is_same<std::pointer_traits<std::shared_ptr<const char>>::element_type,
                           const char>::value, "");
static_assert(std::is_same<std::pointer_traits<Opaque<void>>::element_type, void>::value, "");

static_assert(std::is_same<std::pointer_traits<int*>::element_type, int>::value, "");
static_assert(std::is_same<std::pointer_traits<int*>::rebind<char>, char*>::value, "");

static_assert(HasElement<Fancy<int>>::value, "");
static_assert(!HasElement<int>::value, "");
static_assert(HasElement<NoArgs<int>>::value, "");

template <class Inner> struct Outer {
  template <class Pointer> struct Impl {
    using Pointee = typename std::pointer_traits<
        typename std::remove_cv<typename std::remove_reference<Pointer>::type>::type>::element_type;
  };
};
static_assert(std::is_same<Outer<int>::Impl<const std::unique_ptr<short>&>::Pointee, short>::value, "");

int main() {
  int x = 5;
  Fancy<int, char> f = FT::pointer_to(x);
  if (f.p != &x) return 1;
  int* raw = std::pointer_traits<int*>::pointer_to(x);
  if (raw != &x || *raw != 5) return 2;
  return 0;
}
