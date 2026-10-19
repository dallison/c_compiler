// RUN: -std=c++17
// EXPECT_EXIT: 0
// A member alias template whose pattern names a sibling member alias template
// with its own parameter (`rebind_traits<U> = Traits<rebind_alloc<U>>`, as in
// std::allocator_traits).  The sibling's use must still see the enclosing
// class's arguments when the outer alias is named from outside the class.
#include <memory>

template <class A> struct Traits {
  using const_pointer = const A*;
  using self = Traits;
  template <class U> using rebind_alloc = A;
  template <class U> using rebind_traits = Traits<rebind_alloc<U>>;
};

template <class A, class B> struct same { static constexpr bool value = false; };
template <class A> struct same<A, A> { static constexpr bool value = true; };

struct Sym {
  int v = 7;
  int get() const { return v; }
};

static_assert(same<Traits<Sym>::rebind_traits<int>, Traits<Sym>>::value, "");
static_assert(same<Traits<Sym>::rebind_traits<int>::self, Traits<Sym>>::value,
              "");
static_assert(
    same<Traits<Sym>::rebind_traits<int>::const_pointer, const Sym*>::value,
    "");

template <class T, class Alloc = std::allocator<T>> struct Set {
  using const_pointer = typename std::allocator_traits<
      Alloc>::template rebind_traits<T>::const_pointer;
  struct const_iterator {
    const T* p;
    const_pointer operator->() const { return p; }
  };
  T item;
  const_iterator find() const { return {&item}; }
};

struct Query {
  template <typename It> static int Get(It it) { return it->get(); }
};

int main() {
  Sym s;
  Traits<Sym>::rebind_traits<int>::const_pointer p = &s;
  if (p->get() != 7) return 1;
  Set<Sym> set;
  if (set.find()->get() != 7) return 2;
  if (Query::Get(set.find()) != 7) return 3;
  return 0;
}
