// RUN: -std=c++17 -fconstexpr-eval=audit
// A constexpr constructor that stores `this` in a block-scope static
// constant-initializes that static with its own address.

struct Node {
  const Node* parent;
  int finish = 0;
  int max_count = 7;
  constexpr Node() : parent(this) {}
};

const Node* Empty() {
  alignas(16) static constexpr Node empty_node;
  return &empty_node;
}

template <class T>
struct Link {
  const Link* self;
  T value;
  constexpr explicit Link(T v) : self(this), value(v) {}
};

const Link<long>* Linked() {
  static constexpr Link<long> link(42);
  return &link;
}

int main() { return Empty()->parent == Empty() && Linked()->self == Linked() ? 0 : 1; }
