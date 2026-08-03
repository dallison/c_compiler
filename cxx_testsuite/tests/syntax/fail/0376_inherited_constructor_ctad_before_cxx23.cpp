// RUN: -std=c++20
// EXPECT: Could not deduce template arguments for Derived

template <class T>
struct Base {
  Base(T) {
  }
};

template <class T>
struct Derived : Base<T> {
  using Base<T>::Base;
};

Derived value(42);
