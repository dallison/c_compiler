// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <atomic>
#include <hazard_pointer>
#include <utility>

#if defined(__DAVECC_HAS_GUEST_THREADS__)
#include <thread>
#endif

#if __cpp_lib_hazard_pointer != 202306L
#error "unexpected __cpp_lib_hazard_pointer value"
#endif

struct node;

struct counting_delete {
  void operator()(node* pointer) const noexcept;
};

static int deleted;

struct node : std::hazard_pointer_obj_base<node, counting_delete> {
  int value;
  explicit node(int number) : value(number) {}
};

void counting_delete::operator()(node* pointer) const noexcept {
  ++deleted;
  delete pointer;
}

int main() {
  std::hazard_pointer empty;
  if (!empty.empty()) {
    return 1;
  }

  std::atomic<node*> source(new node(42));
  std::hazard_pointer owner = std::make_hazard_pointer();
  if (owner.empty()) {
    return 2;
  }

  node* protected_node = owner.protect(source);
  if (protected_node == nullptr || protected_node->value != 42) {
    return 3;
  }

  node* removed = source.exchange(nullptr);
  removed->retire(counting_delete());
  if (deleted != 0) {
    return 4;
  }

  std::hazard_pointer moved = static_cast<std::hazard_pointer&&>(owner);
  if (!owner.empty() || moved.empty()) {
    return 5;
  }
  moved.reset_protection();
  if (deleted != 1) {
    return 6;
  }

  node* second = new node(17);
  source.store(second);
  node* candidate = second;
  if (!moved.try_protect(candidate, source) || candidate != second) {
    return 7;
  }
  source.store(nullptr);
  if (moved.try_protect(candidate, source) || candidate != nullptr) {
    return 8;
  }
  second->retire(counting_delete());
  if (deleted != 2) {
    return 9;
  }

#if defined(__DAVECC_HAS_GUEST_THREADS__)
  node* third = new node(99);
  source.store(third);
  if (moved.protect(source) != third) {
    return 10;
  }
  source.store(nullptr);
  std::thread retire_thread([third] {
    third->retire(counting_delete());
  });
  retire_thread.join();
  if (deleted != 2) {
    return 11;
  }
  moved.reset_protection();
  if (deleted != 3) {
    return 12;
  }
#endif

  swap(empty, moved);
  if (empty.empty() || !moved.empty()) {
    return 13;
  }
  return 0;
}
