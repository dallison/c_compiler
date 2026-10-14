// Inside a member of `Ptr<T>`, `Ptr<U>(...)` names Ptr<U>, not the current
// specialization.  std::shared_ptr relies on this to set up
// enable_shared_from_this on an indirect base.
#include <memory>

template <class T>
struct Ptr {
  T* p = nullptr;
  int size = sizeof(T);
  Ptr() = default;
  explicit Ptr(T* q) : p(q) {}
  template <class Y>
  Ptr(const Ptr<Y>&, T* q) : p(q) {}
  Ptr<int> Ints() const { return Ptr<int>(); }
  template <class U>
  Ptr<U> Alias(U* q) const {
    return Ptr<U>(*this, q);
  }
  template <class U>
  int SizeOf() const {
    return Ptr<U>{}.size;
  }
};

struct A {
  int a = 1;
};
struct C : A {
  int c = 2;
};

struct Channel : std::enable_shared_from_this<Channel> {
  virtual ~Channel() = default;
};
struct ClientChannel : Channel {};
struct Publisher : ClientChannel {
  int id = 5;
};

int main() {
  C c;
  Ptr<C> s(&c);
  if (s.Ints().size != sizeof(int)) return 1;
  if (s.Alias<A>(&c).p->a != 1) return 2;
  if (s.SizeOf<double>() != sizeof(double)) return 3;

  std::shared_ptr<Publisher> pub = std::make_shared<Publisher>();
  std::shared_ptr<Channel> self = pub->shared_from_this();
  if (self.get() != pub.get() || pub.use_count() != 2) return 4;
  std::shared_ptr<Publisher> raw(new Publisher);
  if (raw->shared_from_this().get() != raw.get()) return 5;
  return 0;
}
