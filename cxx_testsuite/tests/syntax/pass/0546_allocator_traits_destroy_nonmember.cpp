// RUN: -std=c++17
//
// allocator_traits<Alloc>::destroy must resolve when called from a dependent
// non-member function template helper (the shape used by scoped_allocator).

#include <memory>
#include <type_traits>

template <class T>
struct tag_allocator {
  typedef T value_type;
  template <class U>
  void destroy(U* p) {
    p->~U();
  }
};

struct box {
  int value;
  explicit box(int v) : value(v) {}
};

struct outer {
  tag_allocator<int> alloc_;
  tag_allocator<int>& outer_allocator() { return alloc_; }
};

template <class Adaptor>
struct wrapper : outer {};

namespace std {
namespace detail {

template <class Adaptor, class T>
void adaptor_destroy(Adaptor& self, T* p) {
  allocator_traits<
      typename remove_reference<decltype(self.outer_allocator())>::type>::
      destroy(self.outer_allocator(), p);
}

}  // namespace detail
}  // namespace std

int main() {
  wrapper<int> w;
  box* storage = static_cast<box*>(::operator new(sizeof(box)));
  new (storage) box(42);
  std::detail::adaptor_destroy(w, storage);
  ::operator delete(storage);
  return 0;
}
