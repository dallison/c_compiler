// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <memory>
#include <type_traits>

struct Payload {
  int value;
};

template <class T>
struct PropagatingAllocator {
  using value_type = T;
  using size_type = unsigned long;
  using difference_type = long;
  using pointer = T*;
  using const_pointer = const T*;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;
  using is_always_equal = std::false_type;

  pointer allocate(size_type n) {
    return static_cast<pointer>(::operator new(n * sizeof(T)));
  }

  void deallocate(pointer ptr, size_type n) noexcept {
    ::operator delete(static_cast<void*>(ptr));
  }

  template <class... Args>
  void construct(pointer ptr, Args&&... args) {
    new (ptr) T(std::forward<Args>(args)...);
  }

  void destroy(pointer ptr) {
    ptr->~T();
  }

  size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

int main(void) {
  using PropTraits = std::allocator_traits<PropagatingAllocator<Payload> >;
  if (!PropTraits::propagate_on_container_copy_assignment::value) {
    return 1;
  }
  if (!PropTraits::propagate_on_container_move_assignment::value) {
    return 2;
  }
  if (!PropTraits::propagate_on_container_swap::value) {
    return 3;
  }
  if (PropTraits::is_always_equal::value) {
    return 4;
  }
  return 0;
}
