// RUN: -std=c++20

#include <memory>

template <class Allocator>
struct allocator_property {
  using traits = std::allocator_traits<Allocator>;

  static constexpr bool copy_propagates =
      traits::propagate_on_container_copy_assignment::value;
  static constexpr bool move_propagates =
      traits::propagate_on_container_move_assignment::value;
  static constexpr bool swaps =
      traits::propagate_on_container_swap::value;
  static constexpr bool always_equal = traits::is_always_equal::value;

  void move_assign() noexcept(
      traits::propagate_on_container_move_assignment::value ||
      traits::is_always_equal::value) {}

  void swap() noexcept(traits::propagate_on_container_swap::value ||
                       traits::is_always_equal::value) {
    if constexpr (traits::propagate_on_container_swap::value) {
      static_assert(traits::propagate_on_container_swap::value);
    }
  }
};

using property = allocator_property<std::allocator<int>>;

static_assert(!property::copy_propagates);
static_assert(!property::move_propagates);
static_assert(!property::swaps);
static_assert(property::always_equal);

void instantiate_members(property& value) {
  value.move_assign();
  value.swap();
}
