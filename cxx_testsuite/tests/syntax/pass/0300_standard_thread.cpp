// RUN: -target x86_64 -std=c++20

#include <chrono>
#include <thread>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<std::thread>);
static_assert(!std::is_copy_assignable_v<std::thread>);
static_assert(std::is_move_constructible_v<std::thread>);
static_assert(std::is_move_assignable_v<std::thread>);
static_assert(std::is_default_constructible_v<std::thread::id>);
static_assert(!std::is_copy_constructible_v<std::jthread>);
static_assert(std::is_move_constructible_v<std::jthread>);

void run(int*) {}
void run_stoppable(std::stop_token, int*) {}
static_assert(
    std::is_invocable<decltype(&run_stoppable), std::stop_token, int*>::value);

void check_thread_interface(int* value) {
  std::thread worker(run, value);
  std::thread moved(std::move(worker));
  moved.join();

  std::thread::id id = std::this_thread::get_id();
  (void)id;
  std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  std::jthread automatic(run_stoppable, value);
  (void)automatic.get_stop_token();
  (void)automatic.get_stop_source();
  automatic.request_stop();
}
