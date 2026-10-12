// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class T>
struct Allocator {
  using const_pointer = const T*;

  template <class U>
  struct rebind {
    using other = Allocator<U>;
  };
};

template <class Alloc>
struct AllocatorTraits {
  using const_pointer = typename Alloc::const_pointer;

  template <class T>
  using rebind_traits =
      AllocatorTraits<typename Alloc::template rebind<T>::other>;
};

template <class T, class Alloc = Allocator<int>>
class Container {
 public:
  using allocator_type = Alloc;
  using const_pointer =
      typename AllocatorTraits<Alloc>::template rebind_traits<T>::const_pointer;

  class ConstIterator {
   public:
    explicit ConstIterator(const_pointer pointer) : pointer_(pointer) {}
    const_pointer operator->() const { return pointer_; }

   private:
    const_pointer pointer_;
  };

  explicit Container(const T* pointer) : pointer_(pointer) {}
  ConstIterator begin() const { return ConstIterator(pointer_); }

 private:
  const T* pointer_;
};

struct Value {
  int number;
};

int main() {
  Value value{33};
  Container<Value> values(&value);
  return values.begin()->number == 33 ? 0 : 1;
}
