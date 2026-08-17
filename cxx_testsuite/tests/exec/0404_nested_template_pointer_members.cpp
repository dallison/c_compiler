// RUN: -std=c++26
// EXPECT_EXIT: 0

template <class T>
struct linked_owner {
  struct link {
    link* previous;
    link* next;
    T* value;

    link(link* previous_value = nullptr, link* next_value = nullptr,
         T* element = nullptr)
        : previous(previous_value), next(next_value), value(element) {}
  };

  link end;

  linked_owner() : end(&end, &end, nullptr) {}

  void append(link* node) {
    node->previous = end.previous;
    node->next = &end;
    end.previous->next = node;
    end.previous = node;
  }
};

int main() {
  linked_owner<int> owner;
  linked_owner<int>::link node;
  owner.append(&node);
  if (node.previous != &owner.end || node.next != &owner.end) return 1;
  if (owner.end.previous != &node || owner.end.next != &node) return 2;
  return 0;
}
