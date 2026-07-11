// RUN: -std=c++20
// EXPECT_EXIT: 0

namespace std {
namespace __x_detail {

template <class T>
struct node {
  T value;
};

template <class T>
struct node_type {
  using node_storage = std::__x_detail::node<T>;

  node_storage* node_ptr_;

  explicit node_type(node_storage* p) : node_ptr_(p) {}
  node_type(const node_type& other) : node_ptr_(nullptr) {
    node_ptr_ = other.node_ptr_;
    ((node_type&)other).node_ptr_ = nullptr;
  }
};

}  // namespace __x_detail
}  // namespace std

int main() {
  std::__x_detail::node<int> node;
  node.value = 19;
  std::__x_detail::node_type<int> first(&node);
  std::__x_detail::node_type<int> second = first;
  if (first.node_ptr_ != nullptr || second.node_ptr_ == nullptr) {
    return 1;
  }
  return second.node_ptr_->value == 19 ? 0 : 2;
}
