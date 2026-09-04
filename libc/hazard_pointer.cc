#if !defined(__6502__) && !defined(__W65C02__)

#include <hazard_pointer>
#include <mutex>

namespace std {
namespace __hazard_detail {

struct __slot {
  const void* protection;
  bool active;
  __slot* next;

  __slot(const void* value, bool in_use, __slot* following) noexcept
      : protection(value), active(in_use), next(following) {}
};

struct __registry {
  mutex lock;
  __slot* slots;
  __retired_record* retired;

  __registry() : slots(nullptr), retired(nullptr) {}
};

static __registry& __state() {
  static __registry* state = new __registry;
  return *state;
}

static bool __is_protected(const __registry& state,
                           const void* address) noexcept {
  for (__slot* slot = state.slots; slot != nullptr; slot = slot->next) {
    if (slot->active && slot->protection == address) {
      return true;
    }
  }
  return false;
}

static __retired_record* __collect_ready(__registry& state) noexcept {
  __retired_record* ready = nullptr;
  __retired_record** link = &state.retired;
  while (*link != nullptr) {
    __retired_record* record = *link;
    if (__is_protected(state, record->address)) {
      link = &record->next;
      continue;
    }
    *link = record->next;
    record->next = ready;
    ready = record;
  }
  return ready;
}

static void __reclaim_all(__retired_record* records) noexcept {
  while (records != nullptr) {
    __retired_record* next = records->next;
    records->reclaim(records->context);
    records = next;
  }
}

__slot* __acquire_slot() {
  __registry& state = __state();
  lock_guard<mutex> guard(state.lock);
  for (__slot* slot = state.slots; slot != nullptr; slot = slot->next) {
    if (!slot->active) {
      slot->active = true;
      slot->protection = nullptr;
      return slot;
    }
  }
  __slot* slot = new __slot(nullptr, true, state.slots);
  state.slots = slot;
  return slot;
}

void __release_slot(__slot* slot) noexcept {
  if (slot == nullptr) {
    return;
  }
  __retired_record* ready;
  {
    __registry& state = __state();
    lock_guard<mutex> guard(state.lock);
    slot->protection = nullptr;
    slot->active = false;
    ready = __collect_ready(state);
  }
  __reclaim_all(ready);
}

void __set_protection(__slot* slot, const void* pointer) noexcept {
  if (slot == nullptr) {
    return;
  }
  __retired_record* ready;
  {
    __registry& state = __state();
    lock_guard<mutex> guard(state.lock);
    slot->protection = pointer;
    ready = pointer == nullptr ? __collect_ready(state) : nullptr;
  }
  __reclaim_all(ready);
}

void __retire(__retired_record* record) noexcept {
  if (record == nullptr) {
    return;
  }
  bool reclaim_now = false;
  {
    __registry& state = __state();
    lock_guard<mutex> guard(state.lock);
    if (__is_protected(state, record->address)) {
      record->next = state.retired;
      state.retired = record;
    } else {
      reclaim_now = true;
    }
  }
  if (reclaim_now) {
    record->reclaim(record->context);
  }
}

}  // namespace __hazard_detail
}  // namespace std

#endif
