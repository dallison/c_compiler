// RUN: -std=c++26
// EXPECT: Template-name pack indexing requires C++29

template <typename T>
struct box {};

template <template <typename> typename... Templates>
struct select_first {
  template <typename T>
  using type = Templates...[0]<T>;
};

using selected = select_first<box>::type<int>;
