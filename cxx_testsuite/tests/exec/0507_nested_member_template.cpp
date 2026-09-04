// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <type_traits>

template <class T>
struct outer {
  template <class U>
  struct inner {
    T first;
    U second;
  };
};

template <class T>
auto copy_inner(typename outer<T>::template inner<long> source) {
  using inner_type = typename outer<T>::template inner<long>;
  return inner_type(source);
}

struct holder {
  template <class U>
  struct rebind {};
};

template <class T>
struct transformed_outer {
  using type = outer<T>;
};

template <class T>
auto make_chained_inner(T first) {
  using outer_type = typename transformed_outer<T>::type;
  using inner_type = typename outer_type::template inner<long>;
  inner_type result{first, 9};
  return result;
}

struct nested_type_holder {
  using nested = int;
};

template <class T>
struct outer_alias {
  template <class U>
  struct inner {
    using mapped = typename T::template rebind<U>;
    inner(const mapped&) {}
  };
};

int main() {
  nested_type_holder nested_holder;
  using nested_type = decltype(nested_holder)::nested;
  static_assert(std::is_same<nested_type, int>::value);

  outer<int>::inner<long> source{3, 4};
  auto copy = copy_inner<int>(source);
  auto chained = make_chained_inner(7);

  holder::rebind<long> mapped;
  outer_alias<holder>::inner<long> aliased(mapped);
  (void)aliased;

  return copy.first == 3 && copy.second == 4 &&
                 chained.first == 7 && chained.second == 9
             ? 0
             : 1;
}
