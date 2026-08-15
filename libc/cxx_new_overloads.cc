#include <new>

namespace std {

const nothrow_t nothrow = {};

}  // namespace std

void operator delete(void* ptr, size_t) noexcept {
  ::operator delete(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
  ::operator delete[](ptr);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
  return ::operator new(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
  return ::operator new[](size);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
  ::operator delete(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
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
