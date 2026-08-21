#include <memory_resource>
#include <stdlib.h>
#if !defined(__6502__) && !defined(__W65C02__)
#include <mutex>
#endif

namespace std {
namespace pmr {

namespace __memory_resource_detail {

void* aligned_allocate(size_t bytes, size_t alignment) {
  if (alignment == 0 || (alignment & (alignment - 1)) != 0 ||
      bytes > (size_t)-1 - (alignment - 1)) {
    return nullptr;
  }
  size_t rounded = (bytes + alignment - 1) & ~(alignment - 1);
  return ::aligned_alloc(alignment, rounded);
}

void aligned_deallocate(void* ptr) noexcept {
  ::free(ptr);
}

}  // namespace __memory_resource_detail

memory_resource* new_delete_resource() noexcept {
  static __memory_resource_detail::new_delete_resource_impl resource;
  return &resource;
}

memory_resource* null_memory_resource() noexcept {
  static __memory_resource_detail::null_resource_impl resource;
  return &resource;
}

static memory_resource*& __default_resource_slot() noexcept {
  static memory_resource* resource = new_delete_resource();
  return resource;
}

#if !defined(__6502__) && !defined(__W65C02__)
static mutex& __default_resource_lock() noexcept {
  static mutex lock;
  return lock;
}

static void __lock_default_resource() noexcept {
  __default_resource_lock().lock();
}

static void __unlock_default_resource() noexcept {
  __default_resource_lock().unlock();
}
#endif

memory_resource* set_default_resource(memory_resource* resource) noexcept {
#if !defined(__6502__) && !defined(__W65C02__)
  __lock_default_resource();
#endif
  memory_resource*& slot = __default_resource_slot();
  memory_resource* previous = slot;
  slot = resource != nullptr ? resource : new_delete_resource();
#if !defined(__6502__) && !defined(__W65C02__)
  __unlock_default_resource();
#endif
  return previous;
}

memory_resource* get_default_resource() noexcept {
#if !defined(__6502__) && !defined(__W65C02__)
  __lock_default_resource();
#endif
  memory_resource* resource = __default_resource_slot();
#if !defined(__6502__) && !defined(__W65C02__)
  __unlock_default_resource();
#endif
  return resource;
}

}  // namespace pmr
}  // namespace std
