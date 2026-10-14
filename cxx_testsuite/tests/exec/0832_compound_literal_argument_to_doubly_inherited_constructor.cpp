// When a class has an initializer-list constructor but none is viable, the
// clauses of `T{...}` become ordinary constructor arguments; a typed clause
// such as `Cmp{x}` must still be analyzed before binding to a constructor
// inherited through two using-declarations.
#include <initializer_list>

struct Cmp {
  const int& index;
};

struct Container {
  explicit Container(const Cmp& comp) : cmp(&comp.index) {}
  const int* cmp = nullptr;
};

struct SetContainer : Container {
  using Container::Container;
  SetContainer(std::initializer_list<int> init)
      : Container(Cmp{*init.begin()}) {}
};

struct Set : SetContainer {
  using SetContainer::SetContainer;
};

template <class C> struct TContainer {
  explicit TContainer(const C& comp, int extra = 7)
      : cmp(&comp.index), extra(extra) {}
  const int* cmp;
  int extra;
};

template <class C> struct TSetContainer : TContainer<C> {
  using TContainer<C>::TContainer;
  TSetContainer(std::initializer_list<int>, const C& comp)
      : TContainer<C>(comp, 0) {}
};

template <class C> struct TSet : TSetContainer<C> {
  using Base = TSetContainer<C>;
  using Base::Base;
};

int main() {
  int x = 0;
  Set set{Cmp{x}};
  if (set.cmp != &x) return 1;

  TSet<Cmp> tset{Cmp{x}};
  if (tset.cmp != &x || tset.extra != 7) return 2;

  TSet<Cmp> with_extra{Cmp{x}, 3};
  if (with_extra.cmp != &x || with_extra.extra != 3) return 3;
  return 0;
}
