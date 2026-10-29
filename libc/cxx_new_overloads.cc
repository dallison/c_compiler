#include <new>
#include <stddef.h>
#include <stdlib.h>
#include <__exception_support>

namespace std {

const nothrow_t nothrow = {};

}  // namespace std

// Each replaceable form may be replaced on its own; placement forms may not.
__attribute__((weak)) void operator delete(void* ptr, size_t) noexcept {
  ::operator delete(ptr);
}

__attribute__((weak)) void operator delete[](void* ptr, size_t) noexcept {
  ::operator delete[](ptr);
}

__attribute__((weak)) void* operator new(size_t size,
                                         const std::nothrow_t&) noexcept {
  return ::operator new(size);
}

__attribute__((weak)) void* operator new[](size_t size,
                                           const std::nothrow_t&) noexcept {
  return ::operator new[](size);
}

__attribute__((weak)) void operator delete(void* ptr,
                                           const std::nothrow_t&) noexcept {
  ::operator delete(ptr);
}

__attribute__((weak)) void operator delete[](void* ptr,
                                             const std::nothrow_t&) noexcept {
  ::operator delete[](ptr);
}

void* operator new(size_t, void* ptr) noexcept {
  return ptr;
}

void* operator new[](size_t, void* ptr) noexcept {
  return ptr;
}

void operator delete(void*, void*) noexcept {}

void operator delete[](void*, void*) noexcept {}

#if __cplusplus >= 201703L
// operator new(size_t) is only required to align to
// __STDCPP_DEFAULT_NEW_ALIGNMENT__.  Over-aligned types go through
// aligned_alloc, whose size must be a multiple of the alignment.
static void* AllocateAligned(size_t size, std::align_val_t alignment) {
  size_t align = static_cast<size_t>(alignment);
  if (align < sizeof(void*) || (align & (align - 1)) != 0) {
    return nullptr;
  }
  if (size == 0) {
    size = align;
  } else if (size % align != 0) {
    size_t rounded = (size + align - 1) & ~(align - 1);
    if (rounded < size) {
      return nullptr;
    }
    size = rounded;
  }
  return aligned_alloc(align, size);
}

__attribute__((weak)) void* operator new(size_t size,
                                         std::align_val_t alignment) {
  void* pointer = AllocateAligned(size, alignment);
  if (pointer == nullptr) {
    __DAVECC_THROW(std::bad_alloc());
  }
  return pointer;
}

__attribute__((weak)) void* operator new[](size_t size,
                                           std::align_val_t alignment) {
  return ::operator new(size, alignment);
}

__attribute__((weak)) void operator delete(void* ptr,
                                           std::align_val_t) noexcept {
  ::operator delete(ptr);
}

__attribute__((weak)) void operator delete[](void* ptr,
                                             std::align_val_t) noexcept {
  ::operator delete[](ptr);
}

__attribute__((weak)) void* operator new(size_t size,
                                         std::align_val_t alignment,
                                         const std::nothrow_t&) noexcept {
  return AllocateAligned(size, alignment);
}

__attribute__((weak)) void* operator new[](size_t size,
                                           std::align_val_t alignment,
                                           const std::nothrow_t&) noexcept {
  return AllocateAligned(size, alignment);
}

__attribute__((weak)) void operator delete(void* ptr, std::align_val_t,
                                           const std::nothrow_t&) noexcept {
  ::operator delete(ptr);
}

__attribute__((weak)) void operator delete[](void* ptr, std::align_val_t,
                                             const std::nothrow_t&) noexcept {
  ::operator delete[](ptr);
}

__attribute__((weak)) void operator delete(void* ptr, size_t,
                                           std::align_val_t) noexcept {
  ::operator delete(ptr);
}

__attribute__((weak)) void operator delete[](void* ptr, size_t,
                                             std::align_val_t) noexcept {
  ::operator delete[](ptr);
}
#endif
