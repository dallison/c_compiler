// RUN: -std=c++20
// EXPECT_EXIT: 0

namespace detail {

template <class T>
struct Node {
  T value;
};

template <class T>
struct Handle {
  using node = detail::Node<T>;

  node* node_ptr_;

  explicit Handle(node* p) : node_ptr_(p) {}
  Handle(const Handle& other) : node_ptr_(nullptr) {
    node_ptr_ = other.node_ptr_;
    ((Handle&)other).node_ptr_ = nullptr;
  }
};

}  // namespace detail

int main() {
  detail::Node<int> node;
  node.value = 9;
  detail::Handle<int> first(&node);
  detail::Handle<int> second = first;
  if (first.node_ptr_ != nullptr || second.node_ptr_ == nullptr) {
    return 1;
  }
  return second.node_ptr_->value == 9 ? 0 : 2;
}
