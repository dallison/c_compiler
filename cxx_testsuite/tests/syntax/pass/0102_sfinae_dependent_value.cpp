// RUN: -std=c++20
#include <stddef.h>
#include <type_traits>

template <class It>
typename std::enable_if<!std::is_integral<It>::value, int>::type sfinae_pick(It) {
  return 1;
}

template <class It>
typename std::enable_if<std::is_integral<It>::value, int>::type sfinae_pick(It) {
  return 2;
}

template <class It>
int trait_value(It) {
  return std::is_integral<It>::value ? 2 : 1;
}

template <class T>
struct SFINAEBox {
  SFINAEBox(size_t, const T&) {
  }

  template <class It>
  SFINAEBox(It first, It last,
            typename std::enable_if<!std::is_integral<It>::value, int>::type = 0) {
    T value = *first;
    (void)last;
    (void)value;
  }
};

int main(void) {
  int value = 0;
  int* ptr = &value;
  static_assert(std::is_same<std::remove_reference_t<int&>, int>::value,
                "concrete trait values must still fold");
  int integral_trait = trait_value(value);
  int pointer_trait = trait_value(ptr);
  int integral_pick = sfinae_pick(value);
  int pointer_pick = sfinae_pick(ptr);
  SFINAEBox<int> from_range(ptr, ptr + 1);
  SFINAEBox<int> fill(3, 7);
  (void)integral_trait;
  (void)pointer_trait;
  (void)integral_pick;
  (void)pointer_pick;
  (void)from_range;
  (void)fill;
  return 0;
}
