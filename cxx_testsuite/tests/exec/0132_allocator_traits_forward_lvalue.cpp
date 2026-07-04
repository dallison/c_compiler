// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <memory>

int main(void) {
  std::allocator<int> alloc;
  int storage = 0;
  int value = 23;
  std::allocator_traits<std::allocator<int> >::construct(alloc, &storage, value);
  return storage == 23 ? 0 : 1;
}
