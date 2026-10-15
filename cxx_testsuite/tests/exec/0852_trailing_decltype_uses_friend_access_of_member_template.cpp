// `CommonAccess::GetSlot` is a friend of the node handle, so the private
// `node.slot()` in its trailing return type is accessible when the template
// is deduced from an unrelated caller (absl's btree emplace through a node
// handle).  Checking access from the caller instead broke the return type.
#include <utility>

struct CommonAccess {
  template <typename Node>
  static auto GetSlot(const Node& node) -> decltype(node.slot()) {
    return node.slot();
  }
  template <typename T, typename... Args>
  static T Construct(Args&&... args) {
    return T(typename T::construct_tag_t{}, std::forward<Args>(args)...);
  }
};

template <typename K, typename V>
struct Params {
  using slot_type = std::pair<K, V>;
  template <typename U>
  static auto key(const U& value) -> decltype((value.first)) {
    return value.first;
  }
  static const K& key(const slot_type* s) { return s->first; }
  static const K& key(slot_type* s) { return s->first; }
};

template <typename P>
class NodeHandle {
 public:
  NodeHandle() = default;

 protected:
  friend struct CommonAccess;
  struct construct_tag_t {};
  template <typename... Args>
  NodeHandle(construct_tag_t, Args&&... args)
      : slot_{std::forward<Args>(args)...} {}
  typename P::slot_type* slot() const {
    return const_cast<typename P::slot_type*>(&slot_);
  }

 private:
  typename P::slot_type slot_;
};

template <typename K, typename V>
struct Container {
  using params_type = Params<K, V>;
  using node_type = NodeHandle<params_type>;
  template <typename... Args>
  int emplace(Args&&... args) {
    auto node = CommonAccess::Construct<node_type>(std::forward<Args>(args)...);
    auto* slot = CommonAccess::GetSlot(node);
    return params_type::key(slot) * 10 + slot->second;
  }
};

class Plain {
 public:
  Plain(int a, int b) : slot_{a, b} {}

 private:
  friend struct CommonAccess;
  std::pair<int, int>* slot() const {
    return const_cast<std::pair<int, int>*>(&slot_);
  }
  std::pair<int, int> slot_;
};

int main() {
  Plain plain(3, 4);
  auto* slot = CommonAccess::GetSlot(plain);
  if (slot->first + slot->second != 7) {
    return 1;
  }
  Container<int, int> c;
  return c.emplace(3, 4) == 34 ? 0 : 2;
}
