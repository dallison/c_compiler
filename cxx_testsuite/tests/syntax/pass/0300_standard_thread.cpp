// RUN: -target x86_64 -std=c++20

#include <chrono>
#include <thread>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<std::thread>);
static_assert(!std::is_copy_assignable_v<std::thread>);
static_assert(std::is_move_constructible_v<std::thread>);
static_assert(std::is_move_assignable_v<std::thread>);
static_assert(std::is_default_constructible_v<std::thread::id>);

void run(int*) {}

void check_thread_interface(int* value) {
  std::thread worker(run, value);
  std::thread moved(std::move(worker));
  moved.join();

  std::thread::id id = std::this_thread::get_id();
  (void)id;
  std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
