// RUN: -std=c++17
//
// allocator_traits::construct / destroy fall back to placement new and the
// destructor when Alloc has neither member.

#include <memory>

struct NoConstruct {
  int value;
  explicit NoConstruct(int v) : value(v) {}
};

struct NotAnAllocator {};

int main() {
  alignas(NoConstruct) unsigned char storage[sizeof(NoConstruct)];
  NoConstruct* ptr = reinterpret_cast<NoConstruct*>(storage);
  NotAnAllocator alloc;
  std::allocator_traits<NotAnAllocator>::construct(alloc, ptr, 7);
  if (ptr->value != 7) {
    return 1;
  }
  std::allocator_traits<NotAnAllocator>::destroy(alloc, ptr);
  return 0;
}
