// A member function template of a class template whose trailing return type
// calls through a data member (`decltype(obj_ptr->Bump(t))`) resolves against
// the class specialization's member, not the pattern's dependent one.
#include <stdio.h>
#include <functional>
#include <utility>

struct Foo {
  int n = 0;
  void Bump() { n += 1; }
  int Bump(int k) { return n += k; }
  long Twice(int k) const { return 2L * k; }
};

template <class Class, typename MethodPtr>
struct InvokeMethodAction {
  Class* const obj_ptr;
  const MethodPtr method_ptr;
  template <typename... Args>
  auto operator()(Args&&... args) const
      -> decltype((obj_ptr->*method_ptr)(std::forward<Args>(args)...)) {
    return (obj_ptr->*method_ptr)(std::forward<Args>(args)...);
  }
};

template <class C>
struct Caller {
  C* obj_ptr;
  template <typename T>
  auto Call(T t) const -> decltype(obj_ptr->Bump(t)) {
    return obj_ptr->Bump(t);
  }
  template <typename T>
  auto CallThis(T t) -> decltype(this->obj_ptr->Twice(t)) {
    return this->obj_ptr->Twice(t);
  }
};

int main() {
  Foo f;
  InvokeMethodAction<Foo, void (Foo::*)()> bump{&f, &Foo::Bump};
  bump();
  std::invoke(bump);
  InvokeMethodAction<Foo, int (Foo::*)(int)> add{&f, &Foo::Bump};
  int after_add = add(10);
  Caller<Foo> c{&f};
  int r = c.Call(100);
  long twice = c.CallThis(21);
  if (f.n != 112 || after_add != 12 || r != 112 || twice != 42) {
    printf("%d %d %d %ld\n", f.n, after_add, r, twice);
    return 1;
  }
  printf("ok\n");
  return 0;
}
