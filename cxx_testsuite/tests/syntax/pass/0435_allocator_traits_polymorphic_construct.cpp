// RUN: -std=c++20

#include <memory>
#include <memory_resource>

void construct_with_polymorphic_allocator(
    std::pmr::polymorphic_allocator<int>& allocator, int* pointer) {
  std::allocator_traits<std::pmr::polymorphic_allocator<int>>::construct(
      allocator, pointer, 42);
  std::allocator_traits<std::pmr::polymorphic_allocator<int>>::destroy(
      allocator, pointer);
}

template <class Allocator>
struct allocator_owner {
  using traits = std::allocator_traits<Allocator>;

  Allocator allocator;

  template <class... Args>
  void construct(int* pointer, Args&&... args) {
    traits::construct(allocator, pointer, std::forward<Args>(args)...);
  }
};

using polymorphic_owner =
    allocator_owner<std::pmr::polymorphic_allocator<int>>;
static_assert(std::is_same_v<
              polymorphic_owner::traits,
              std::allocator_traits<std::pmr::polymorphic_allocator<int>>>);

void construct_from_member_template(
    allocator_owner<std::pmr::polymorphic_allocator<int>>& owner,
    int* pointer) {
  owner.construct(pointer, 42);
}
