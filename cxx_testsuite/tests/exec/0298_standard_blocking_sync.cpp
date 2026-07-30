// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <atomic>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <latch>
#include <mutex>
#include <semaphore>
#include <thread>

struct phase_completion {
  std::atomic<int>* phases;

  void operator()() noexcept {
    phases->fetch_add(1);
  }
};

void lock_until_scope_exit(std::mutex& mutex) {
  std::lock_guard<std::mutex> guard(mutex);
}

int main() {
  std::mutex mutex;
  lock_until_scope_exit(mutex);
  if (!mutex.try_lock()) {
    return 13;
  }
  mutex.unlock();

  std::condition_variable condition;
  int value = 0;
  std::thread condition_worker([&] {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&] { return value != 0; });
    value += 2;
  });
  {
    std::lock_guard<std::mutex> lock(mutex);
    value = 5;
  }
  condition.notify_one();
  condition_worker.join();
  if (value != 7) {
    return 1;
  }

  int exit_value = 0;
  std::thread exit_notifier([&] {
    std::unique_lock<std::mutex> lock(mutex);
    exit_value = 9;
    std::notify_all_at_thread_exit(condition, std::move(lock));
  });
  {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&] { return exit_value == 9; });
  }
  exit_notifier.join();

  {
    std::unique_lock<std::mutex> lock(mutex);
    if (condition.wait_for(lock, std::chrono::milliseconds(1)) !=
        std::cv_status::timeout) {
      return 2;
    }
    if (condition.wait_for(lock, std::chrono::milliseconds(1),
                           [] { return false; })) {
      return 3;
    }
  }
  std::recursive_mutex recursive;
  std::condition_variable_any any_condition;
  std::atomic<int> any_waiting{0};
  bool any_ready = false;
  std::thread any_worker([&] {
    std::unique_lock<std::recursive_mutex> lock(recursive);
    any_waiting.store(1);
    any_condition.wait(lock);
    any_ready = false;
  });
  while (any_waiting.load() == 0) {
    std::this_thread::yield();
  }
  {
    std::lock_guard<std::recursive_mutex> lock(recursive);
    any_ready = true;
  }
  any_condition.notify_all();
  any_worker.join();
  if (any_ready) {
    return 4;
  }
  std::counting_semaphore<3> permits(0);
  std::atomic<int> semaphore_value{0};
  std::thread semaphore_worker([&] {
    semaphore_value.store(11);
    permits.release();
  });
  permits.acquire();
  semaphore_worker.join();
  if (semaphore_value.load() != 11 || permits.try_acquire()) {
    return 5;
  }
  if (permits.try_acquire_for(std::chrono::milliseconds(1))) {
    return 6;
  }
  permits.release(2);
  if (!permits.try_acquire() || !permits.try_acquire() ||
      permits.try_acquire()) {
    return 7;
  }

  std::binary_semaphore binary(1);
  if (!binary.try_acquire() || binary.try_acquire()) {
    return 8;
  }
  binary.release();
  binary.acquire();
  std::latch finished(2);
  std::atomic<int> latch_value{0};
  std::thread latch_first([&] {
    latch_value.fetch_add(1);
    finished.count_down();
  });
  std::thread latch_second([&] {
    latch_value.fetch_add(2);
    finished.count_down();
  });
  finished.wait();
  latch_first.join();
  latch_second.join();
  if (!finished.try_wait() || latch_value.load() != 3) {
    return 9;
  }
  std::atomic<int> phases{0};
  std::atomic<int> barrier_value{0};
  std::barrier<phase_completion> phase(2, phase_completion{&phases});
  std::thread barrier_worker([&] {
    barrier_value.store(1);
    phase.arrive_and_wait();
    barrier_value.fetch_add(2);
    phase.arrive_and_wait();
  });
  while (barrier_value.load() == 0) {
    std::this_thread::yield();
  }
  phase.arrive_and_wait();
  if (phases.load() != 1) {
    return 10;
  }
  phase.arrive_and_wait();
  barrier_worker.join();
  if (phases.load() != 2 || barrier_value.load() != 3) {
    return 11;
  }
  std::atomic<int> drop_phases{0};
  std::barrier<phase_completion> dropping(
      2, phase_completion{&drop_phases});
  std::thread dropping_worker([&] { dropping.arrive_and_drop(); });
  dropping.arrive_and_wait();
  dropping_worker.join();
  dropping.arrive_and_wait();
  if (drop_phases.load() != 2) {
    return 12;
  }

  return 0;
}
