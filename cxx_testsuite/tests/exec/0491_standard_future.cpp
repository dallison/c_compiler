// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>
#include <future>
#include <thread>

int add(int left, int right) {
  return left + right;
}

int square(int value) {
  return value * value;
}

int main() {
  std::promise<int> promise;
  std::future<int> result = promise.get_future();
  std::thread producer([&] { promise.set_value(7); });
  if (result.get() != 7) return 1;
  producer.join();

  std::future<int> asynchronous = std::async(add, 2, 3);
  if (asynchronous.get() != 5) return 2;

  std::future<int> deferred =
      std::async(std::launch::deferred, square, 4);
  if (deferred.wait_for(std::chrono::milliseconds(0)) !=
      std::future_status::deferred) {
    return 3;
  }
  if (deferred.get() != 16) return 4;

  std::packaged_task<int(int)> task(square);
  std::future<int> task_result = task.get_future();
  task(5);
  if (task_result.get() != 25) return 5;

  std::shared_future<int> shared = std::async(square, 6).share();
  std::shared_future<int> shared_copy = shared;
  if (shared.get() != 36 || shared_copy.get() != 36) return 6;

  std::future<int> broken;
  {
    std::promise<int> abandoned;
    broken = abandoned.get_future();
  }
  try {
    (void)broken.get();
    return 7;
  } catch (const std::future_error& error) {
    if (error.code() !=
        std::make_error_code(std::future_errc::broken_promise)) {
      return 8;
    }
  }

  int value = 9;
  std::promise<int&> reference_promise;
  std::future<int&> reference = reference_promise.get_future();
  reference_promise.set_value(value);
  reference.get() = 10;
  if (value != 10) return 9;

  std::promise<void> void_promise;
  std::future<void> completion = void_promise.get_future();
  void_promise.set_value();
  completion.get();
  return 0;
}
