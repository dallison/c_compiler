// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <memory>

template <class T>
struct traits {
  static T choose(const T& value) {
    return value;
  }
};

template <class T>
struct box {
  using traits_type = traits<T>;
  T value;

  box(const T& v) : value(traits_type::choose(v)) {}

  T choose_again(const T& v) {
    return traits_type::choose(v);
  }
};

template <class T>
struct alloc_box {
  using allocator_type = std::allocator<T>;
  using traits_type = std::allocator_traits<allocator_type>;
  allocator_type alloc;

  T* allocate_one() {
    return traits_type::allocate(alloc, 1);
  }

  void release_one(T* ptr) {
    traits_type::deallocate(alloc, ptr, 1);
  }
};

template <class T>
struct rebound_alloc_box {
  struct node {
    T value;
  };

  using value_allocator = std::allocator<T>;
  using value_traits = std::allocator_traits<value_allocator>;
  using node_allocator = typename value_traits::template rebind_alloc<node>;
  using node_traits = std::allocator_traits<node_allocator>;

  node_allocator alloc;

  node* allocate_one() {
    return node_traits::allocate(alloc, 1);
  }

  void release_one(node* ptr) {
    node_traits::deallocate(alloc, ptr, 1);
  }
};

int main() {
  box<int> b(42);
  if (b.value != 42) {
    return 1;
  }
  if (b.choose_again(7) != 7) {
    return 2;
  }
  alloc_box<int> ab;
  int* ptr = ab.allocate_one();
  *ptr = 11;
  if (*ptr != 11) {
    return 3;
  }
  ab.release_one(ptr);
  rebound_alloc_box<int> rb;
  rebound_alloc_box<int>::node* node = rb.allocate_one();
  node->value = 13;
  if (node->value != 13) {
    return 4;
  }
  rb.release_one(node);
  return 0;
}
