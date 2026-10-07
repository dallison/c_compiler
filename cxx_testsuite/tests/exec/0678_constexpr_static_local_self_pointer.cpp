// RUN: -std=c++17
// EXPECT_EXIT: 0
// A block-scope static whose constexpr constructor stores `this` (Abseil's
// btree EmptyNode) must point at itself at run time.

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

constexpr Node Make() { return Node(); }

const Node* Made() {
  static constexpr Node made = Make();
  return &made;
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

int main() {
  const Node* empty = Empty();
  if (empty->parent != empty || empty->max_count != 7) {
    return 1;
  }
  const Node* made = Made();
  if (made->parent != made || made == empty) {
    return 2;
  }
  const Link<long>* link = Linked();
  if (link->self != link || link->value != 42) {
    return 3;
  }
  return 0;
}
