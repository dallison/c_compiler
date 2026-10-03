// An alias template whose pattern is a dependent decltype, used as a template
// argument in a qualified call, must bind the alias's own parameter rather than
// the enclosing template's parameter at the same index.

template <class T>
T&& declval() noexcept;

template <class T>
using ref_t = decltype(*declval<T&>());

template <class U>
struct Invoker {
  static U call(U value) { return value; }
};

template <class A, class T>
int first_param_unrelated(A, T* p) {
  return Invoker<ref_t<T*>>::call(*p);
}

template <class T>
auto deduced(T* p) {
  return Invoker<ref_t<T*>>::call(*p);
}

int main() {
  int value = 3;
  if (first_param_unrelated(1.5, &value) != 3) {
    return 1;
  }
  if (deduced(&value) != 3) {
    return 2;
  }
  return 0;
}
