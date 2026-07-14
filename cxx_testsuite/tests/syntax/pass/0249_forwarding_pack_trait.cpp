// RUN: -std=c++20

#include <type_traits>

struct Target {
  Target(int, long) {}
};

static_assert(std::is_same<const int&, const int&>::value);
static_assert(std::is_same<int&&, int&&>::value);
static_assert(std::is_const<std::remove_reference_t<const int&> >::value);
static_assert(!std::is_object<int&>::value);

template <class T>
T explicit_return_type();

using ExplicitConstReference = decltype(explicit_return_type<const int&>());
static_assert(std::is_reference<ExplicitConstReference>::value);
static_assert(std::is_const<
              std::remove_reference_t<ExplicitConstReference> >::value);

void consume(int, long);

template <class T, class... Args>
  requires (std::is_copy_constructible<T>::value &&
            std::is_constructible<T, Args&&...>::value)
void construct_from_forwarding_pack(Args&&... args) {
  consume(static_cast<Args&&>(args)...);
}

void forwarding_pack_trait() {
  construct_from_forwarding_pack<Target>(1, 2L);
}
