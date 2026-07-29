// RUN: -std=c++20

#include <atomic>
#include <type_traits>

#ifndef __cpp_lib_atomic_ref
#error "atomic_ref feature-test macro is missing"
#endif

static_assert(__cpp_lib_atomic_ref >= 201806L);
static_assert(std::is_same_v<std::atomic_ref<int>::value_type, int>);
static_assert(std::atomic_ref<int>::required_alignment == alignof(int));

void use_atomic_ref(int& value) {
  std::atomic_ref<int> reference(value);
  int expected = 0;
  reference.compare_exchange_strong(expected, 1,
                                    std::memory_order_release);
  reference.fetch_add(2, std::memory_order_relaxed);
}
