#include <rcu>

#if defined(__DAVECC_HAS_GUEST_THREADS__) && \
    !defined(__6502__) && !defined(__W65C02__)

#include <mutex>
#include <shared_mutex>

namespace std {
namespace __rcu_detail {

struct __domain_state {
  shared_mutex readers;
  mutex queue_guard;
  mutex reclaim_guard;
  __retired_record* head = nullptr;
  __retired_record* tail = nullptr;
  size_t next_ticket = 0;
  size_t completed_ticket = 0;
};

static thread_local size_t __reader_depth;

void* __default_state() noexcept {
  static __domain_state* state = new __domain_state;
  return state;
}

void __lock(void* handle) noexcept {
  __domain_state& state = *static_cast<__domain_state*>(handle);
  if (__reader_depth++ == 0) {
    state.readers.lock_shared();
  }
}

bool __try_lock(void* handle) noexcept {
  __domain_state& state = *static_cast<__domain_state*>(handle);
  if (__reader_depth != 0) {
    ++__reader_depth;
    return true;
  }
  if (!state.readers.try_lock_shared()) {
    return false;
  }
  __reader_depth = 1;
  return true;
}

void __unlock(void* handle) noexcept {
  __domain_state& state = *static_cast<__domain_state*>(handle);
  if (--__reader_depth == 0) {
    state.readers.unlock_shared();
  }
}

void __synchronize(void* handle) noexcept {
  __domain_state& state = *static_cast<__domain_state*>(handle);
  state.readers.lock();
  state.readers.unlock();
}

void __barrier(void* handle) noexcept {
  __domain_state& state = *static_cast<__domain_state*>(handle);
  size_t target;
  {
    lock_guard<mutex> lock(state.queue_guard);
    target = state.next_ticket;
    if (state.completed_ticket >= target) {
      return;
    }
  }

  lock_guard<mutex> reclaimer(state.reclaim_guard);
  if (state.completed_ticket >= target) {
    return;
  }

  __synchronize(handle);

  __retired_record* ready;
  __retired_record* ready_tail = nullptr;
  {
    lock_guard<mutex> lock(state.queue_guard);
    ready = state.head;
    for (__retired_record* record = ready;
         record != nullptr && record->ticket <= target;
         record = record->next) {
      ready_tail = record;
    }
    if (ready_tail == nullptr) {
      return;
    }
    state.head = ready_tail->next;
    ready_tail->next = nullptr;
    if (state.head == nullptr) {
      state.tail = nullptr;
    }
  }

  while (ready != nullptr) {
    __retired_record* next = ready->next;
    size_t ticket = ready->ticket;
    ready->reclaim(ready->context);
    {
      lock_guard<mutex> lock(state.queue_guard);
      state.completed_ticket = ticket;
    }
    ready = next;
  }
}

void __schedule_state(__retired_record* record, void* handle) noexcept {
  __domain_state& state = *static_cast<__domain_state*>(handle);
  lock_guard<mutex> lock(state.queue_guard);
  record->next = nullptr;
  record->ticket = ++state.next_ticket;
  if (state.tail != nullptr) {
    state.tail->next = record;
  } else {
    state.head = record;
  }
  state.tail = record;
}

}  // namespace __rcu_detail
}  // namespace std

#endif /* atomic/thread support */
