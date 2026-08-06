#include <memory_resource>
#if !defined(__6502__) && !defined(__W65C02__)
#include <mutex>
#endif

namespace std {
namespace pmr {

namespace __memory_resource_detail {

struct aligned_allocation {
  void* pointer;
  void* raw;
  aligned_allocation* next;
};

static aligned_allocation*& aligned_allocations() noexcept {
  static aligned_allocation* allocations;
  return allocations;
}

#if !defined(__6502__) && !defined(__W65C02__)
static mutex& aligned_allocations_lock() noexcept {
  static mutex lock;
  return lock;
}
#endif

void* aligned_allocate(size_t bytes, size_t alignment) {
  char* raw =
      static_cast<char*>(::operator new(bytes + alignment - 1));
  uintptr_t address = reinterpret_cast<uintptr_t>(raw);
  size_t remainder = address % alignment;
  char* pointer =
      remainder == 0 ? raw : raw + (alignment - remainder);

  aligned_allocation* record = nullptr;
#ifdef __cpp_exceptions
  try {
    record = static_cast<aligned_allocation*>(
        ::operator new(sizeof(aligned_allocation)));
  } catch (...) {
    ::operator delete(raw);
    throw;
  }
#else
  record = static_cast<aligned_allocation*>(
      ::operator new(sizeof(aligned_allocation)));
#endif
  record->pointer = pointer;
  record->raw = raw;
#if !defined(__6502__) && !defined(__W65C02__)
  aligned_allocations_lock().lock();
#endif
  record->next = aligned_allocations();
  aligned_allocations() = record;
#if !defined(__6502__) && !defined(__W65C02__)
  aligned_allocations_lock().unlock();
#endif
  return pointer;
}

void aligned_deallocate(void* ptr) noexcept {
#if !defined(__6502__) && !defined(__W65C02__)
  aligned_allocations_lock().lock();
#endif
  aligned_allocation** link = &aligned_allocations();
  while (*link != nullptr && (*link)->pointer != ptr) {
    link = &(*link)->next;
  }
  aligned_allocation* record = *link;
  if (record != nullptr) {
    *link = record->next;
  }
#if !defined(__6502__) && !defined(__W65C02__)
  aligned_allocations_lock().unlock();
#endif
  if (record != nullptr) {
    ::operator delete(record->raw);
    ::operator delete(record);
  }
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
