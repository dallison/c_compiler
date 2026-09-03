// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <thread>
#include <utility>

int main() {
  std::shared_mutex mutex;
  mutex.lock_shared();
  mutex.unlock_shared();
  if (!mutex.try_lock()) return 1;
  if (mutex.try_lock_shared()) return 2;
  mutex.unlock();

  std::shared_lock<std::shared_mutex> first(mutex);
  if (!first.owns_lock() || first.mutex() != &mutex) return 3;
  std::shared_lock<std::shared_mutex> second(std::move(first));
  if (first.owns_lock() || !second.owns_lock()) return 4;
  second.unlock();

  std::shared_timed_mutex timed;
  if (!timed.try_lock_for(std::chrono::milliseconds(1))) return 5;
  timed.unlock();
  if (!timed.try_lock_shared_for(std::chrono::milliseconds(1))) return 6;
  timed.unlock_shared();

  std::atomic<bool> started(false);
  std::atomic<bool> acquired(false);
  mutex.lock_shared();
  std::thread writer([&] {
    started.store(true);
    mutex.lock();
    acquired.store(true);
    mutex.unlock();
  });
  while (!started.load()) std::this_thread::yield();
  for (int i = 0; i < 10; ++i) std::this_thread::yield();
  if (acquired.load()) return 7;
  mutex.unlock_shared();
  writer.join();
  if (!acquired.load()) return 8;
  return 0;
}
