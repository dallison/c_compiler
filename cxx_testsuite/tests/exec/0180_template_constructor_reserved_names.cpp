// RUN: -std=c++20
// EXPECT_EXIT: 0

namespace __detail {

template <class T>
struct __node {
  T value;
};

template <class T>
struct __node_type {
  using __node_alias = __detail::__node<T>;

  __node_alias* node_ptr_;

  explicit __node_type(__node_alias* p) : node_ptr_(p) {}
  __node_type(const __node_type& other) : node_ptr_(nullptr) {
    node_ptr_ = other.node_ptr_;
    ((__node_type&)other).node_ptr_ = nullptr;
  }
};

}  // namespace __detail

int main() {
  __detail::__node<int> node;
  node.value = 15;
  __detail::__node_type<int> first(&node);
  __detail::__node_type<int> second = first;
  if (first.node_ptr_ != nullptr || second.node_ptr_ == nullptr) {
    return 1;
  }
  return second.node_ptr_->value == 15 ? 0 : 2;
}
