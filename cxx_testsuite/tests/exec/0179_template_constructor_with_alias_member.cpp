// RUN: -std=c++20
// EXPECT_EXIT: 0

namespace detail {

template <class T>
struct Node {
  T value;
};

template <class T>
struct Alloc {
  int marker;
  Alloc() : marker(5) {}
  Alloc& operator=(const Alloc& other) {
    marker = other.marker;
    return *this;
  }
};

template <class T>
struct Handle {
  using node = detail::Node<T>;
  using alloc_type = detail::Alloc<node>;

  node* node_ptr_;
  alloc_type alloc_;

  explicit Handle(node* p) : node_ptr_(p), alloc_() {}
  Handle(const Handle& other) : node_ptr_(nullptr), alloc_() {
    node_ptr_ = other.node_ptr_;
    alloc_ = other.alloc_;
    ((Handle&)other).node_ptr_ = nullptr;
  }
  ~Handle() {
    if (node_ptr_ != nullptr) {
      node_ptr_ = nullptr;
    }
  }
};

}  // namespace detail

int main() {
  detail::Node<int> node;
  node.value = 13;
  detail::Handle<int> first(&node);
  detail::Handle<int> second = first;
  if (first.node_ptr_ != nullptr || second.node_ptr_ == nullptr) {
    return 1;
  }
  if (second.alloc_.marker != 5) {
    return 2;
  }
  return second.node_ptr_->value == 13 ? 0 : 3;
}
