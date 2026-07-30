// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <atomic>
#include <chrono>
#include <functional>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>

struct move_only_function {
  int* value;

  explicit move_only_function(int* destination) : value(destination) {}
  move_only_function(const move_only_function&) = delete;
  move_only_function& operator=(const move_only_function&) = delete;
  move_only_function(move_only_function&& other) : value(other.value) {
    other.value = nullptr;
  }

  void operator()(int increment) {
    *value += increment;
  }
};

struct receiver {
  int value = 0;

  void add(int increment) {
    value += increment;
  }
};

struct tracked_callable {
  std::atomic<int>* calls;
  std::atomic<int>* destructions;
  bool owns;

  tracked_callable(std::atomic<int>* call_count,
                   std::atomic<int>* destruction_count)
      : calls(call_count), destructions(destruction_count), owns(true) {}
  tracked_callable(const tracked_callable&) = delete;
  tracked_callable& operator=(const tracked_callable&) = delete;
  tracked_callable(tracked_callable&& other)
      : calls(other.calls), destructions(other.destructions), owns(other.owns) {
    other.owns = false;
  }
  ~tracked_callable() {
    if (owns) {
      destructions->fetch_add(1);
    }
  }

  void operator()() {
    calls->fetch_add(1);
  }
};

int main() {
  static_assert(!std::is_copy_constructible_v<std::thread>);
  static_assert(!std::is_copy_assignable_v<std::thread>);
  static_assert(std::is_move_constructible_v<std::thread>);
  static_assert(std::is_move_assignable_v<std::thread>);

  int value = 1;
  std::thread first(move_only_function(&value), 4);
  if (!first.joinable() || first.get_id() == std::thread::id{}) {
    return 1;
  }

  std::thread::id first_id = first.get_id();
  std::thread moved(std::move(first));
  if (first.joinable() || !moved.joinable() || moved.get_id() != first_id) {
    return 2;
  }
  moved.join();
  if (moved.joinable() || value != 5) {
    return 3;
  }

  receiver object;
  std::thread member(&receiver::add, &object, 7);
  member.join();
  if (object.value != 7) {
    return 4;
  }

  std::atomic<unsigned long> child_id_hash{0};
  std::thread identify([&] {
    child_id_hash.store(std::hash<std::thread::id>{}(
        std::this_thread::get_id()));
  });
  unsigned long expected_hash =
      std::hash<std::thread::id>{}(identify.get_id());
  identify.join();
  if (child_id_hash.load() != expected_hash) {
    return 5;
  }

  std::atomic<int> detached_done{0};
  std::thread detached([&] {
    detached_done.store(1);
    detached_done.notify_one();
  });
  detached.detach();
  if (detached.joinable()) {
    return 6;
  }
  while (detached_done.load() == 0) {
    detached_done.wait(0);
  }

  bool join_error = false;
  try {
    std::thread empty;
    empty.join();
  } catch (const std::system_error& error) {
    join_error =
        error.code() == std::make_error_code(std::errc::invalid_argument);
  }
  if (!join_error) {
    return 7;
  }

  auto before = std::chrono::steady_clock::now();
  std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  auto after_sleep = std::chrono::steady_clock::now();
  if (after_sleep < before + std::chrono::milliseconds(1)) {
    return 8;
  }
  std::this_thread::sleep_until(after_sleep + std::chrono::milliseconds(1));
  if (std::chrono::steady_clock::now() < after_sleep) {
    return 9;
  }

  if (std::thread::hardware_concurrency() == 0) {
    return 10;
  }

  std::thread assigned;
  std::thread source([] {});
  assigned = std::move(source);
  if (source.joinable() || !assigned.joinable()) {
    return 11;
  }
  assigned.join();

  std::atomic<int> tracked_calls{0};
  std::atomic<int> tracked_destructions{0};
  std::thread tracked{
      tracked_callable(&tracked_calls, &tracked_destructions)};
  tracked.join();
  if (tracked_calls.load() != 1 || tracked_destructions.load() != 1) {
    return 12;
  }

  receiver zero_initialized{0};
  if (zero_initialized.value != 0) {
    return 13;
  }

  std::error_code default_error;
  if (default_error || default_error.message().empty()) {
    return 14;
  }

  return 0;
}
