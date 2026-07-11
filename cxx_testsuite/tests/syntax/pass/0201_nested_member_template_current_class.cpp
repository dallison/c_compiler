// RUN: -std=c++20
template <class T>
struct outer {
  template <class U>
  struct rebind {
    using other = outer<U>;
  };
};

using rebound = typename outer<int>::template rebind<long>::other;
