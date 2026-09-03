// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <scoped_allocator>
#include <memory>
#include <type_traits>

template <int Tag, class T>
struct TagAllocator {
  int tag;
  typedef T value_type;
  typedef unsigned long size_type;
  typedef long difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;

  TagAllocator(int value = Tag) : tag(value) {}

  TagAllocator(const TagAllocator& other) : tag(other.tag) {}
  template <class U>
  TagAllocator(const TagAllocator<Tag, U>& other) : tag(other.tag) {}

  pointer allocate(size_type n) {
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  void deallocate(pointer ptr, size_type) {
    ::operator delete(static_cast<void*>(ptr));
  }

  template <class U, class... Args>
  void construct(U* ptr, Args&&... args) {
    new (ptr) U(std::forward<Args>(args)...);
  }

  template <class U>
  void destroy(U* ptr) {
    ptr->~U();
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) { ptr->~T(); }

  size_type max_size() const { return static_cast<size_type>(-1) / sizeof(T); }

  TagAllocator select_on_container_copy_construction() const {
    return TagAllocator(tag + 100);
  }
};

template <int T1, class A, int T2, class B>
bool operator==(const TagAllocator<T1, A>& left,
                const TagAllocator<T2, B>& right) {
  return left.tag == right.tag;
}

template <int T1, class A, int T2, class B>
bool operator!=(const TagAllocator<T1, A>& left,
                const TagAllocator<T2, B>& right) {
  return !(left == right);
}

template <class Alloc>
struct PrefixContainer {
  typedef Alloc allocator_type;
  int value;
  int alloc_tag;

  PrefixContainer(std::allocator_arg_t, const Alloc& alloc, int v)
      : value(v), alloc_tag(alloc.outer_allocator().tag) {}

  PrefixContainer(int v) : value(v), alloc_tag(-1) {}
};

template <class Alloc>
struct SuffixContainer {
  typedef Alloc allocator_type;
  int value;
  int alloc_tag;

  SuffixContainer(int v, const Alloc& alloc)
      : value(v), alloc_tag(alloc.outer_allocator().tag) {}

  SuffixContainer(int v) : value(v), alloc_tag(-1) {}
};

struct Plain {
  int value;
  explicit Plain(int v) : value(v) {}
};

int main() {
  typedef TagAllocator<1, int> OuterRaw;
  typedef TagAllocator<2, int> InnerRaw;
  typedef std::scoped_allocator_adaptor<InnerRaw> InnerAdapt;
  typedef std::scoped_allocator_adaptor<OuterRaw, InnerRaw> Scoped;

  OuterRaw outer(10);
  InnerRaw inner(20);
  Scoped scoped(outer, inner);

  if (scoped.outer_allocator().tag != 10) {
    return 1;
  }
  if (scoped.inner_allocator().outer_allocator().tag != 20) {
    return 2;
  }

  PrefixContainer<InnerAdapt>* prefix_storage =
      static_cast<PrefixContainer<InnerAdapt>*>(
          ::operator new(sizeof(PrefixContainer<InnerAdapt>)));
  scoped.construct(prefix_storage, 42);
  if (prefix_storage->value != 42 || prefix_storage->alloc_tag != 20) {
    return 11;
  }
  scoped.destroy(prefix_storage);
  ::operator delete(prefix_storage);

  typedef std::scoped_allocator_adaptor<OuterRaw> SingleScoped;
  SingleScoped single(OuterRaw(30));
  if (single.outer_allocator().tag != 30) {
    return 3;
  }
  if (single.inner_allocator().outer_allocator().tag != 30) {
    return 4;
  }

  Scoped copied = scoped.select_on_container_copy_construction();
  if (copied.outer_allocator().tag != 110) {
    return 5;
  }
  if (copied.inner_allocator().outer_allocator().tag != 120) {
    return 6;
  }

  typedef std::allocator_traits<Scoped> ScopedTraits;
  if (ScopedTraits::propagate_on_container_copy_assignment::value) {
    return 7;
  }
  if (ScopedTraits::propagate_on_container_move_assignment::value) {
    return 8;
  }
  if (ScopedTraits::propagate_on_container_swap::value) {
    return 9;
  }
  if (ScopedTraits::is_always_equal::value) {
    return 10;
  }

  SuffixContainer<InnerAdapt>* suffix_storage =
      static_cast<SuffixContainer<InnerAdapt>*>(
          ::operator new(sizeof(SuffixContainer<InnerAdapt>)));
  scoped.construct(suffix_storage, 55);
  if (suffix_storage->value != 55 || suffix_storage->alloc_tag != 20) {
    return 12;
  }
  scoped.destroy(suffix_storage);
  ::operator delete(suffix_storage);

  Plain* plain_storage =
      static_cast<Plain*>(::operator new(sizeof(Plain)));
  scoped.construct(plain_storage, 77);
  if (plain_storage->value != 77) {
    return 13;
  }
  scoped.destroy(plain_storage);
  ::operator delete(plain_storage);

  Scoped same(OuterRaw(10), InnerRaw(20));
  if (!(scoped == same)) {
    return 14;
  }
  if (scoped != same) {
    return 15;
  }

  Scoped different(OuterRaw(11), InnerRaw(20));
  if (scoped == different) {
    return 16;
  }
  if (!(scoped != different)) {
    return 17;
  }

  return 0;
}
