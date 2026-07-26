// RUN: -std=c++20

#include <chrono>
#include <mutex>

void mutex_surface() {
  std::mutex first;
  std::mutex second;
  std::recursive_mutex recursive;
  std::timed_mutex timed;
  std::recursive_timed_mutex recursive_timed;

  std::lock_guard<std::mutex> guard(first);
  std::unique_lock<std::mutex> deferred(second, std::defer_lock);
  deferred.lock();
  deferred.unlock();
  std::scoped_lock<std::mutex, std::mutex> both(first, second);

  recursive.lock();
  recursive.unlock();
  (void)timed.try_lock_for(std::chrono::milliseconds(1));
  (void)recursive_timed.try_lock_until(
      std::chrono::steady_clock::now() + std::chrono::milliseconds(1));
}

std::once_flag flag;

void once_surface() {
  std::call_once(flag, [] {});
}
