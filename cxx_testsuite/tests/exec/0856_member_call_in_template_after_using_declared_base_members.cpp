// A class template whose `using Base::empty;` members precede its own member
// templates (protobuf's Map) lays the specialization's members out in another
// order than the pattern.  A `this->CreateNode(...)` call cloned into a member
// template instantiation must still name CreateNode, not whatever member sits
// in the pattern's slot in the specialization.
#include <utility>

struct UntypedBase {
  int num_elements_ = 0;
  bool empty() const { return num_elements_ == 0; }
  int size() const { return num_elements_; }
};

template <typename Key>
struct KeyBase : UntypedBase {};

template <typename Key, typename T>
class Map : private KeyBase<Key> {
  using Base = KeyBase<Key>;

 public:
  struct Node {
    Key first;
    T second;
  };
  using Base::empty;
  using Base::size;

  T& operator[](const Key& key) { return try_emplace(key)->second; }
  template <typename K, typename... Args>
  Node* try_emplace(K&& k, Args&&... args) {
    return TryEmplaceInternal(std::forward<K>(k), std::forward<Args>(args)...);
  }
  const Node& at(int i) const { return storage[i]; }

 private:
  Node storage[4];
  template <typename K, typename... Args>
  Node* CreateNode(K&& k, Args&&... args) {
    Node* node = &storage[this->num_elements_];
    node->first = std::forward<K>(k);
    node->second = T(std::forward<Args>(args)...);
    return node;
  }
  template <typename K, typename... Args>
  Node* TryEmplaceInternal(K&& k, Args&&... args) {
    auto* node = CreateNode(std::forward<K>(k), std::forward<Args>(args)...);
    ++this->num_elements_;
    return node;
  }
};

int main() {
  Map<int, int> m;
  if (!m.empty()) {
    return 1;
  }
  m[3] = 9;
  m.try_emplace(4, 5);
  if (m.size() != 2) {
    return 2;
  }
  return m.at(0).first == 3 && m.at(0).second == 9 && m.at(1).first == 4 &&
                 m.at(1).second == 5
             ? 0
             : 3;
}
