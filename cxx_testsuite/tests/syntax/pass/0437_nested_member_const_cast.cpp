// RUN: -std=c++20

template <class T>
struct owner {
  struct node {
    T value;
  };

  node sentinel;

  node* mutable_sentinel() const {
    return const_cast<node*>(&sentinel);
  }
};

void use_nested_const_cast(const owner<int>& value) {
  (void)value.mutable_sentinel();
}
