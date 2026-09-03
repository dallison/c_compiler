// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// scoped_allocator_adaptor::construct must automatically propagate the inner
// allocator to uses_allocator types even when the outer allocator type also
// provides a generic construct template.

#include <memory>
#include <scoped_allocator>
#include <type_traits>

template <int Tag, class T>
struct TagAllocator {
  int tag;
  typedef T value_type;
  TagAllocator(int value = Tag) : tag(value) {}

  template <class U, class... Args>
  void construct(U* ptr, Args&&... args) {
    new (ptr) U(std::forward<Args>(args)...);
  }

  template <class U>
  void destroy(U* ptr) {
    ptr->~U();
  }
};

template <class Alloc>
struct PrefixContainer {
  typedef Alloc allocator_type;
  int value;
  int alloc_tag;

  PrefixContainer(std::allocator_arg_t, const Alloc& alloc, int v)
      : value(v), alloc_tag(alloc.outer_allocator().tag) {}
};

template <class Alloc>
struct SuffixContainer {
  typedef Alloc allocator_type;
  int value;
  int alloc_tag;

  SuffixContainer(int v, const Alloc& alloc)
      : value(v), alloc_tag(alloc.outer_allocator().tag) {}
};

int main() {
  typedef TagAllocator<1, int> OuterRaw;
  typedef TagAllocator<2, int> InnerRaw;
  typedef std::scoped_allocator_adaptor<InnerRaw> InnerAdapt;
  typedef std::scoped_allocator_adaptor<OuterRaw, InnerRaw> Scoped;

  Scoped scoped(OuterRaw(10), InnerRaw(20));

  PrefixContainer<InnerAdapt>* prefix_storage =
      static_cast<PrefixContainer<InnerAdapt>*>(
          ::operator new(sizeof(PrefixContainer<InnerAdapt>)));
  scoped.construct(prefix_storage, 42);
  if (prefix_storage->value != 42 || prefix_storage->alloc_tag != 20) {
    return 1;
  }
  scoped.destroy(prefix_storage);
  ::operator delete(prefix_storage);

  SuffixContainer<InnerAdapt>* suffix_storage =
      static_cast<SuffixContainer<InnerAdapt>*>(
          ::operator new(sizeof(SuffixContainer<InnerAdapt>)));
  scoped.construct(suffix_storage, 55);
  if (suffix_storage->value != 55 || suffix_storage->alloc_tag != 20) {
    return 2;
  }
  scoped.destroy(suffix_storage);
  ::operator delete(suffix_storage);

  return 0;
}
