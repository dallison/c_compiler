// RUN: -std=c++20

#include <coroutine>
#include <stdint.h>

// The frame prefix ends at 24 bytes on LP64. A 16-byte-aligned promise starts
// on the next 16-byte boundary, not at sizeof(prefix).

struct alignas(16) Promise;

struct Task {
  using promise_type = Promise;
  std::coroutine_handle<Promise> handle;
};

struct alignas(16) Promise {
  int value;
  char pad[16];

  Task get_return_object() {
    return {std::coroutine_handle<Promise>::from_promise(*this)};
  }

  std::suspend_always initial_suspend() noexcept { return {}; }

  std::suspend_always final_suspend() noexcept { return {}; }

  void return_void() noexcept {}

  void unhandled_exception() noexcept {}
};

Task aligned_promise() {
  co_return;
}

int main() {
  static_assert(alignof(Promise) == 16, "alignas did not reach alignof");
  Task task = aligned_promise();
  if (!task.handle) {
    return 1;
  }
  Promise& promise = task.handle.promise();
  uintptr_t frame = reinterpret_cast<uintptr_t>(task.handle.address());
  uintptr_t promise_address = reinterpret_cast<uintptr_t>(&promise);
  if (promise_address < frame) {
    return 2;
  }
  // Distance must be the aligned offset, not the raw prefix size. A wrong
  // from_promise shifts the handle, so this difference is independent of
  // the two functions agreeing with each other.
  if ((promise_address - frame) % alignof(Promise) != 0) {
    return 3;
  }
  if (promise_address % alignof(Promise) != 0) {
    return 4;
  }
  std::coroutine_handle<Promise> again =
      std::coroutine_handle<Promise>::from_promise(promise);
  if (again.address() != task.handle.address() || &again.promise() != &promise) {
    return 5;
  }
  task.handle.destroy();
  return 0;
}
