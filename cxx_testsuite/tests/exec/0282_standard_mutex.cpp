// RUN: -std=c++20

#include <chrono>
#include <mutex>

static int failures;

static void check(bool condition) {
  if (!condition) ++failures;
}

int main() {
  std::mutex first;
  std::mutex second;

  {
    std::lock_guard<std::mutex> guard(first);
    check(!first.try_lock());
  }
  check(first.try_lock());
  first.unlock();

  {
    std::unique_lock<std::mutex> lock(first, std::defer_lock);
    check(!lock.owns_lock());
    lock.lock();
    check(lock.owns_lock());
    lock.unlock();
    check(!lock.owns_lock());
  }

  std::lock(first, second);
  {
    std::lock_guard<std::mutex> one(first, std::adopt_lock);
    std::lock_guard<std::mutex> two(second, std::adopt_lock);
  }

  {
    std::scoped_lock<std::mutex, std::mutex> both(first, second);
    check(!first.try_lock());
    check(!second.try_lock());
  }

  std::recursive_mutex recursive;
  recursive.lock();
  check(recursive.try_lock());
  recursive.unlock();
  recursive.unlock();

  std::timed_mutex timed;
  timed.lock();
  check(!timed.try_lock_for(std::chrono::microseconds(100)));
  check(!timed.try_lock_until(std::chrono::steady_clock::now() +
                              std::chrono::microseconds(100)));
  timed.unlock();
  check(timed.try_lock_for(std::chrono::microseconds(100)));
  timed.unlock();

  std::recursive_timed_mutex recursive_timed;
  recursive_timed.lock();
  check(recursive_timed.try_lock_for(std::chrono::microseconds(100)));
  recursive_timed.unlock();
  recursive_timed.unlock();

  std::once_flag once;
  int attempts = 0;
#ifdef __cpp_exceptions
  try {
    std::call_once(once, [&] {
      ++attempts;
      throw 7;
    });
  } catch (int value) {
    check(value == 7);
  }
#endif
  std::call_once(once, [&] { ++attempts; });
  std::call_once(once, [&] { ++attempts; });
#ifdef __cpp_exceptions
  check(attempts == 2);
#else
  check(attempts == 1);
#endif

  return failures;
}
