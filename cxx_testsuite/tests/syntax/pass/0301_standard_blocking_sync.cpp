// RUN: -target x86_64 -std=c++20

#include <barrier>
#include <chrono>
#include <condition_variable>
#include <latch>
#include <mutex>
#include <semaphore>

static_assert(__cpp_lib_semaphore == 201907L);
static_assert(__cpp_lib_latch == 201907L);
static_assert(__cpp_lib_barrier == 201907L);

struct completion {
  void operator()() noexcept {}
};

void check_blocking_sync_interfaces() {
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  std::condition_variable condition;
  condition.notify_one();
  condition.wait_for(lock, std::chrono::milliseconds(1), [] { return true; });

  std::condition_variable_any any_condition;
  any_condition.notify_all();
  any_condition.wait_for(lock, std::chrono::milliseconds(1),
                         [] { return true; });

  std::counting_semaphore<4> semaphore(1);
  semaphore.acquire();
  semaphore.release();
  (void)semaphore.try_acquire_for(std::chrono::milliseconds(1));

  std::binary_semaphore binary(1);
  (void)binary.try_acquire();

  std::latch latch(1);
  latch.count_down();
  latch.wait();

  std::barrier<completion> barrier(1, completion{});
  barrier.arrive_and_wait();

  std::barrier deduced_barrier(1, completion{});
  deduced_barrier.arrive_and_wait();
}

void check_notify_at_exit(std::condition_variable& condition,
                          std::unique_lock<std::mutex> lock) {
  std::notify_all_at_thread_exit(condition, std::move(lock));
}
