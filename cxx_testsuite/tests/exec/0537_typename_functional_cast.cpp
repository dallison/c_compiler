// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <utility>

struct Node {
  struct transfer_tag_t {};
  struct construct_tag_t {};
  int v;
  Node(transfer_tag_t, int x) : v(x) {}
  Node(construct_tag_t, int x) : v(x) {}
};

struct CommonAccess {
  template <typename T, typename... Args>
  static T Transfer(Args&&... args) {
    return T(typename T::transfer_tag_t{}, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  static T Construct(Args&&... args) {
    return T(typename T::construct_tag_t{}, std::forward<Args>(args)...);
  }
};

int main() {
  Node a = CommonAccess::Transfer<Node>(3);
  Node b = CommonAccess::Construct<Node>(4);
  return a.v == 3 && b.v == 4 ? 0 : 1;
}
