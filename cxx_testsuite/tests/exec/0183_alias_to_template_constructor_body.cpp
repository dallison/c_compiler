// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <memory>
#include <utility>

namespace std {
namespace __x3_detail {

template <class Key, class T>
struct node {
  pair<const Key, T> value;
};

template <class Key, class T, class Allocator>
struct node_type {
  using node_storage = std::__x3_detail::node<Key, T>;

  node_storage* node_ptr_;

  explicit node_type(node_storage* p) : node_ptr_(p) {}
  node_type(const node_type& other) : node_ptr_(nullptr) {
    node_ptr_ = other.node_ptr_;
    ((node_type&)other).node_ptr_ = nullptr;
  }
};

}  // namespace __x3_detail

template <class Key, class T,
          class Allocator = allocator<pair<const Key, T> > >
struct owner {
  using node_type = __x3_detail::node_type<Key, T, Allocator>;
};

}  // namespace std

int main() {
  std::__x3_detail::node<int, int> node;
  std::owner<int, int>::node_type first(&node);
  std::owner<int, int>::node_type second = first;
  if (first.node_ptr_ != nullptr || second.node_ptr_ == nullptr) {
    return 1;
  }
  return 0;
}
