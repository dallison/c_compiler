// RUN: -std=c++17 -fconstexpr-eval=audit
// A block-scope static initialized from a call returning an object whose
// constructor stored `this`.  The folded initializer, `{&made, ...}`, takes the
// address of a static, which both evaluators must accept.

struct Node {
  const Node* parent;
  int finish = 0;
  constexpr Node() : parent(this) {}
};

constexpr Node Make() { return Node(); }

const Node* Made() {
  static constexpr Node made = Make();
  return &made;
}

int main() { return Made()->parent == Made() ? 0 : 1; }
