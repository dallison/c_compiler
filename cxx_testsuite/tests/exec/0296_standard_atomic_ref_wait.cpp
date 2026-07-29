// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <atomic>
#include <threads.h>

std::atomic_int counter(0);
int intrinsic_counter = 0;
int gate = 0;

int increment_counter(void*) {
  for (int i = 0; i < 1000; ++i) {
    counter.fetch_add(1, std::memory_order_relaxed);
    __atomic_fetch_add(&intrinsic_counter, 1, 0);
  }
  return 0;
}

int wait_for_gate(void*) {
  std::atomic_ref<int> reference(gate);
  reference.wait(0, std::memory_order_acquire);
  return reference.load(std::memory_order_relaxed) == 1 ? 0 : 1;
}

int main() {
  static_assert(std::atomic_ref<int>::is_always_lock_free);
  static_assert(std::atomic_ref<int>::required_alignment == alignof(int));

  std::atomic_ref<int> reference(gate);
  if (!reference.is_lock_free() || reference.exchange(0) != 0) {
    return 1;
  }
  if (reference.fetch_or(6) != 0 || gate != 6 ||
      reference.fetch_and(3) != 6 || gate != 2 ||
      reference.fetch_xor(3) != 2 || gate != 1) {
    return 7;
  }
  reference.store(0, std::memory_order_relaxed);

  int values[4] = {};
  std::atomic<int*> pointer(values);
  if (pointer.fetch_add(2) != values || pointer.load() != values + 2 ||
      pointer.fetch_sub(1) != values + 2 || pointer.load() != values + 1) {
    return 8;
  }
  int* raw_pointer = values;
  std::atomic_ref<int*> pointer_reference(raw_pointer);
  if (pointer_reference.fetch_add(3) != values ||
      raw_pointer != values + 3) {
    return 9;
  }

  thrd_t first;
  thrd_t second;
  if (thrd_create(&first, increment_counter, nullptr) != thrd_success ||
      thrd_create(&second, increment_counter, nullptr) != thrd_success) {
    return 2;
  }
  int first_result = 0;
  int second_result = 0;
  if (thrd_join(first, &first_result) != thrd_success ||
      thrd_join(second, &second_result) != thrd_success) {
    return 3;
  }
  if (first_result != 0 || second_result != 0 ||
      counter.load(std::memory_order_relaxed) != 2000 ||
      intrinsic_counter != 2000) {
    return 4;
  }

  thrd_t waiter;
  if (thrd_create(&waiter, wait_for_gate, nullptr) != thrd_success) {
    return 5;
  }
  reference.store(1, std::memory_order_release);
  reference.notify_one();
  int wait_result = 0;
  if (thrd_join(waiter, &wait_result) != thrd_success || wait_result != 0) {
    return 6;
  }

  std::atomic_int free_wait(1);
  std::atomic_wait_explicit(&free_wait, 0, std::memory_order_relaxed);
  std::atomic_notify_all(&free_wait);
  return 0;
}
