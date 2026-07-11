// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <memory>
#include <utility>

namespace std {
namespace __x2_detail {

template <class Key, class T>
struct node {
  pair<const Key, T> value;
};

template <class Key, class T, class Allocator>
struct node_type {
  using node_storage = std::__x2_detail::node<Key, T>;

  node_storage* node_ptr_;

  explicit node_type(node_storage* p) : node_ptr_(p) {}
  node_type(const node_type& other) : node_ptr_(nullptr) {
    node_ptr_ = other.node_ptr_;
    ((node_type&)other).node_ptr_ = nullptr;
  }
};

}  // namespace __x2_detail
}  // namespace std

int main() {
  std::__x2_detail::node<int, int> node;
  std::__x2_detail::node_type<
      int, int, std::allocator<std::pair<const int, int> > > first(&node);
  std::__x2_detail::node_type<
      int, int, std::allocator<std::pair<const int, int> > > second = first;
  if (first.node_ptr_ != nullptr || second.node_ptr_ == nullptr) {
    return 1;
  }
  return 0;
}
