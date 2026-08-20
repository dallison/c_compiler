// RUN: -std=c++29
// EXPECT: pack index 2 is out of bounds for a pack of length 1

template <typename T>
struct box {};

template <template <typename> typename... Templates>
struct select_template {
  template <typename T>
  using type = Templates...[2]<T>;
};

using selected = select_template<box>::type<int>;
