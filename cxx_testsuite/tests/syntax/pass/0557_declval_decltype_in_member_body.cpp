// RUN: -std=c++20

// Instantiating a member function whose body names a class template specialized
// on `decltype(*declval<T&>())` must not require a definition of `declval`.

template <class T>
T&& declval() noexcept;

template <class T>
using iter_reference_t = decltype(*declval<T&>());

template <class U>
struct Invoker {
  static U call(U value) { return value; }
};

template <class T>
struct View {
  T* ptr;

  auto operator*() const {
    return Invoker<iter_reference_t<T>>::call(*ptr);
  }
};

int check_declval_decltype_member_body() {
  int value = 7;
  View<int> view{&value};
  return *view;
}
