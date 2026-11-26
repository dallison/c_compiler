// Failed nothrow new / new[] must return null without writing a cookie or
// initializing through the allocation pointer.
#include <new>
#include <stddef.h>

void* operator new(size_t) {
  return nullptr;
}

void* operator new[](size_t) {
  return nullptr;
}

void operator delete(void*) noexcept {}
void operator delete[](void*) noexcept {}

int main() {
  int* p = new (std::nothrow) int(7);
  if (p != nullptr) {
    return 1;
  }
  int* a = new (std::nothrow) int[64];
  if (a != nullptr) {
    return 2;
  }
  int* z = new (std::nothrow) int[0];
  if (z != nullptr) {
    return 3;
  }
  return 0;
}
