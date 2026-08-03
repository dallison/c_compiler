// RUN: -std=c++23

template <class T>
struct Base {
  T value;

  Base(T initial) : value(initial) {
  }
};

template <class T>
struct Derived : Base<T> {
  using Base<T>::Base;
};

int main(void) {
  Derived value(42);
  return value.value == 42 ? 0 : 1;
}
