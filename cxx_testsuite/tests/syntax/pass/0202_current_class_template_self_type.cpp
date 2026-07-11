// RUN: -std=c++20
template <class T>
struct self_ref {
  using self = self_ref<T>;
};
