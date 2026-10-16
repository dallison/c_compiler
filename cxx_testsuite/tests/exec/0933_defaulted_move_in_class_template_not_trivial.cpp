// A move constructor defaulted on its first declaration in a class template is
// trivial only when the instantiation's members are trivially movable.  It was
// decided against the dependent pattern, so a `std::function` member was moved
// bitwise and the moved-from object still owned the callable.
#include <stdio.h>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

struct Resource {
  Resource() : p(new int(1)) {}
  Resource(Resource&& other) : p(other.p) { other.p = nullptr; }
  Resource(const Resource& other) : p(new int(*other.p)) {}
  ~Resource() { delete p; }
  int* p;
};

template <class T> struct Holder {
  Holder() = default;
  Holder(const Holder&) = delete;
  Holder(Holder&&) = default;
  T member;
};

template <class F> class Once;
template <class R, class... Args> class Once<R(Args...)> final {
 public:
  template <class C, typename std::enable_if<
                         !std::is_same<Once, typename std::decay<C>::type>::value,
                         int>::type = 0>
  Once(C&& callable)  // NOLINT
      : function_(Adaptor<typename std::decay<C>::type>(
            0, std::forward<C>(callable))) {}
  Once(const Once&) = delete;
  Once(Once&&) = default;
  R Call(Args... args) && { return function_(std::forward<Args>(args)...); }

 private:
  template <class Callable> struct Adaptor {
    template <class F>
    Adaptor(int, F&& f) : callable_(std::make_shared<Callable>(std::forward<F>(f))) {}
    R operator()(Args... args) const { return (*callable_)(args...); }
    std::shared_ptr<Callable> callable_;
  };
  std::function<R(Args...)> function_;
};

struct State {
  explicit State(int v) : value(new int(v)) {}
  const std::shared_ptr<int> value;
  int operator()() const { return *value; }
};

std::shared_ptr<Once<int()>> Keep(Once<int()> once) {
  return std::make_shared<Once<int()>>(std::move(once));
}

int main() {
  int failures = 0;
  if (std::is_trivially_move_constructible<Holder<Resource>>::value) failures |= 1;
  if (!std::is_trivially_move_constructible<Holder<int>>::value) failures |= 2;
  {
    Holder<Resource> a;
    Holder<Resource> b(std::move(a));
    if (a.member.p != nullptr || b.member.p == nullptr) failures |= 4;
  }
  std::weak_ptr<int> weak;
  std::shared_ptr<Once<int()>> kept;
  {
    State state(19);
    weak = state.value;
    kept = Keep(state);
  }
  if (weak.expired()) failures |= 8;
  if (std::move(*kept).Call() != 19) failures |= 16;
  kept.reset();
  if (!weak.expired()) failures |= 32;
  if (failures == 0) printf("ok\n");
  return failures;
}
