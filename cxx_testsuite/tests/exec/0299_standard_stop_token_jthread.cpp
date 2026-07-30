// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <atomic>
#include <chrono>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>

struct blocking_callback {
  std::atomic<int>* entered;
  std::atomic<int>* release;

  void operator()() noexcept {
    entered->store(1);
    entered->notify_all();
    while (release->load() == 0) {
      release->wait(0);
    }
  }
};

struct stopping_worker {
  std::atomic<int>* observed;

  void operator()(std::stop_token stop) {
    while (!stop.stop_requested()) {
      std::this_thread::yield();
    }
    if (observed != nullptr) {
      observed->store(1);
    }
  }
};

struct self_deleting_callback {
  std::atomic<int>* calls;

  void operator()() noexcept;
};

using self_registration = std::stop_callback<self_deleting_callback>;
static self_registration* self_registration_owner;
void run_function(int* calls) {
  *calls = 1;
}
static_assert(
    !std::is_invocable<decltype(&run_function), std::stop_token, int*>::value);

void run_stoppable_function(std::stop_token stop,
                            std::atomic<int>* observed) {
  while (!stop.stop_requested()) {
    std::this_thread::yield();
  }
  observed->store(1);
}

void self_deleting_callback::operator()() noexcept {
  if (self_registration_owner == nullptr) {
    calls->store(99);
    return;
  }
  calls->fetch_add(1);
  delete self_registration_owner;
  self_registration_owner = nullptr;
}

int main() {
  static_assert(__cpp_lib_jthread == 201911L);
  static_assert(!std::is_copy_constructible_v<std::stop_callback<void (*)()>>);
  static_assert(std::is_move_constructible_v<std::jthread>);

  std::stop_source source;
  std::stop_token token = source.get_token();
  if (!source.stop_possible() || !token.stop_possible() ||
      source.stop_requested() || token.stop_requested()) {
    return 1;
  }

  std::atomic<int> calls{0};
  {
    std::stop_callback first(token, [&] { calls.fetch_add(1); });
    std::stop_callback second(token, [&] { calls.fetch_add(1); });
    if (!source.request_stop() || source.request_stop()) {
      return 2;
    }
    if (calls.load() != 2 || !token.stop_requested()) {
      return 3;
    }
    std::stop_callback late(token, [&] { calls.fetch_add(1); });
    if (calls.load() != 3) {
      return 4;
    }
  }

  std::stop_source disabled(std::nostopstate);
  if (disabled.stop_possible() || disabled.request_stop() ||
      disabled.get_token().stop_possible()) {
    return 5;
  }

  std::stop_token orphan;
  {
    std::stop_source temporary;
    orphan = temporary.get_token();
  }
  if (orphan.stop_possible() || orphan.stop_requested()) {
    return 6;
  }

  std::stop_source removable_source;
  std::atomic<int> removed_calls{0};
  {
    std::stop_callback removed(removable_source.get_token(),
                               [&] { removed_calls.fetch_add(1); });
  }
  removable_source.request_stop();
  if (removed_calls.load() != 0) {
    return 7;
  }

  std::stop_source blocking_source;
  std::atomic<int> entered{0};
  std::atomic<int> release{0};
  using blocking_registration = std::stop_callback<blocking_callback>;
  unsigned long blocking_storage[
      (sizeof(blocking_registration) + sizeof(unsigned long) - 1) /
      sizeof(unsigned long)];
  blocking_registration* blocking = new ((void*)blocking_storage)
      blocking_registration(
      blocking_source.get_token(), blocking_callback{&entered, &release});
  std::thread requester([&] { blocking_source.request_stop(); });
  while (entered.load() == 0) {
    entered.wait(0);
  }

  std::atomic<int> delete_started{0};
  std::atomic<int> delete_finished{0};
  std::thread destroyer([&] {
    delete_started.store(1);
    delete_started.notify_all();
    blocking->~blocking_registration();
    delete_finished.store(1);
    delete_finished.notify_all();
  });
  while (delete_started.load() == 0) {
    delete_started.wait(0);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  if (delete_finished.load() != 0) {
    return 8;
  }
  release.store(1);
  release.notify_all();
  requester.join();
  destroyer.join();
  if (delete_finished.load() != 1) {
    return 9;
  }

  std::stop_source self_source;
  std::atomic<int> self_calls{0};
  std::stop_token self_token = self_source.get_token();
  if (self_source.stop_requested() || self_token.stop_requested()) {
    return 20;
  }
  self_registration_owner = new self_registration(
      self_token,
      self_deleting_callback{&self_calls});
  if (self_registration_owner == nullptr) {
    return 18;
  }
  if (self_calls.load() != 0) {
    return 19;
  }
  self_source.request_stop();
  if (self_calls.load() != 1) {
    return 17;
  }
  if (self_registration_owner != nullptr) {
    return 10;
  }
  for (int iteration = 0; iteration < 20; iteration++) {
    std::stop_source racing_source;
    std::atomic<int> racing_calls{0};
    std::atomic<int> begin{0};
    std::thread racing_request([&] {
      while (begin.load() == 0) {
        begin.wait(0);
      }
      racing_source.request_stop();
    });
    begin.store(1);
    begin.notify_all();
    {
      std::stop_callback racing_callback(
          racing_source.get_token(),
          [&] { racing_calls.fetch_add(1); });
      racing_request.join();
      if (racing_calls.load() != 1) {
        return 11;
      }
    }
  }
  std::atomic<int> observed_stop{0};
  {
    std::jthread automatic(stopping_worker{&observed_stop});
    if (!automatic.joinable() || !automatic.get_stop_token().stop_possible()) {
      return 12;
    }
  }
  if (observed_stop.load() != 1) {
    return 13;
  }
  int plain_value = 0;
  {
    std::jthread plain([&] { plain_value = 7; });
  }
  if (plain_value != 7) {
    return 14;
  }
  int function_calls = 0;
  {
    std::jthread function_thread(&run_function, &function_calls);
    if (!function_thread.joinable()) {
      return 25;
    }
  }
  if (function_calls != 1) {
    return 23;
  }
  std::atomic<int> function_stop{0};
  std::jthread stoppable_function_thread(
      &run_stoppable_function, &function_stop);
  stoppable_function_thread.request_stop();
  stoppable_function_thread.join();
  if (function_stop.load() != 1) {
    return 24;
  }

  observed_stop.store(0);
  std::jthread assignment_target(stopping_worker{&observed_stop});
  std::atomic<int> moved_stop{0};
  std::jthread assignment_source(stopping_worker{&moved_stop});
  std::jthread moved(std::move(assignment_source));
  assignment_target = std::move(moved);
  if (observed_stop.load() != 1 || moved.joinable() ||
      !assignment_target.joinable()) {
    return 15;
  }
  assignment_target.request_stop();
  assignment_target.join();
  if (moved_stop.load() != 1) {
    return 21;
  }

  observed_stop.store(0);
  std::jthread explicit_stop(stopping_worker{&observed_stop});
  if (!explicit_stop.request_stop() || explicit_stop.request_stop()) {
    return 16;
  }
  explicit_stop.join();
  if (observed_stop.load() != 1) {
    return 22;
  }

  return 0;
}
