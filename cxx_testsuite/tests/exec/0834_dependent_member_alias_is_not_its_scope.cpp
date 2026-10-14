// An alias for `typename Traits<T>::iterator` must not be reused when the
// class later names `Traits<T>` itself: `Traits<T>::kind` is looked up in
// Traits, not in the iterator type.
template <typename T>
struct Traits;

template <typename T>
struct Iter {
  T* p;
};

template <typename T>
class Ref {
  typedef typename Traits<T>::iterator IteratorType;

 public:
  Ref() : kind(Traits<T>::kind) {}
  int kind;
};

template <typename T>
struct Traits {
  typedef Iter<T> iterator;
  static constexpr int kind = 10;
};

struct Message {};

int main() {
  Ref<Message> r;
  return r.kind == 10 ? 0 : 1;
}
