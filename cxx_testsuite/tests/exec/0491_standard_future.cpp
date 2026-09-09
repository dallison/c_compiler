// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>
#include <future>
#include <memory>
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

  std::promise<int> first_swap;
  std::promise<int> second_swap;
  std::future<int> first_future = first_swap.get_future();
  std::swap(first_swap, second_swap);
  second_swap.set_value(12);
  if (first_future.get() != 12) return 10;

  std::promise<int> swap_src;
  std::future<int> swap_left = swap_src.get_future();
  std::future<int> swap_right;
  swap_left.swap(swap_right);
  if (swap_left.valid() || !swap_right.valid()) return 20;
  swap_src.set_value(13);
  if (swap_right.get() != 13) return 21;

  try {
    throw std::future_error(std::future_errc::no_state);
  } catch (const std::future_error& error) {
    if (error.code() !=
        std::make_error_code(std::future_errc::no_state)) {
      return 11;
    }
  }

  if (!std::uses_allocator<std::promise<int>, std::allocator<char>>::value) {
    return 12;
  }
  if (!std::uses_allocator<std::packaged_task<int(int)>,
                           std::allocator<char>>::value) {
    return 19;
  }

  std::promise<int> delayed;
  std::future<int> delayed_value = delayed.get_future();
  std::promise<void> setter_started;
  std::promise<void> allow_exit;
  std::thread delayed_worker([&] {
    delayed.set_value_at_thread_exit(42);
    setter_started.set_value();
    allow_exit.get_future().wait();
  });
  setter_started.get_future().wait();
  if (delayed_value.wait_for(std::chrono::milliseconds(0)) !=
      std::future_status::timeout) {
    return 13;
  }
  allow_exit.set_value();
  delayed_worker.join();
  if (delayed_value.get() != 42) return 14;

  std::packaged_task<int(int)> delayed_task(square);
  std::future<int> delayed_task_result = delayed_task.get_future();
  std::promise<void> task_started;
  std::promise<void> task_allow_exit;
  std::thread task_worker([&] {
    delayed_task.make_ready_at_thread_exit(7);
    task_started.set_value();
    task_allow_exit.get_future().wait();
  });
  task_started.get_future().wait();
  if (delayed_task_result.wait_for(std::chrono::milliseconds(0)) !=
      std::future_status::timeout) {
    return 15;
  }
  task_allow_exit.set_value();
  task_worker.join();
  if (delayed_task_result.get() != 49) return 16;

  std::promise<int> satisfied;
  std::future<int> satisfied_future = satisfied.get_future();
  int satisfied_status = 0;
  std::thread already([&] {
    satisfied.set_value_at_thread_exit(1);
    try {
      satisfied.set_value(2);
      satisfied_status = 1;
    } catch (const std::future_error& error) {
      if (error.code() !=
          std::make_error_code(std::future_errc::promise_already_satisfied)) {
        satisfied_status = 2;
      }
    }
  });
  already.join();
  if (satisfied_status != 0) return 17;
  if (satisfied_future.get() != 1) return 18;

  return 0;
}
