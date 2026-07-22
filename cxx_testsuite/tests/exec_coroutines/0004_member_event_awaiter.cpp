// RUN: -std=c++20

#include <coroutine>

struct Promise;

struct Task {
  using promise_type = Promise;
  std::coroutine_handle<Promise> handle;
};

struct Promise {
  Task get_return_object() {
    return {std::coroutine_handle<Promise>::from_promise(*this)};
  }

  std::suspend_always initial_suspend() noexcept {
    return {};
  }

  std::suspend_always final_suspend() noexcept {
    return {};
  }

  void return_void() noexcept {
  }

  void unhandled_exception() noexcept {
  }
};

struct Event {
  struct Awaiter {
    Event* event;

    bool await_ready() const noexcept {
      return event->signaled;
    }

    void await_suspend(std::coroutine_handle<> coroutine) noexcept {
      event->waiter = coroutine;
    }

    void await_resume() const noexcept {
    }
  };

  bool signaled;
  std::coroutine_handle<> waiter;

  Awaiter operator co_await() noexcept {
    return {this};
  }

  void signal() noexcept {
    signaled = true;
    if (waiter) {
      std::coroutine_handle<> coroutine = waiter;
      waiter = {};
      coroutine.resume();
    }
  }
};

struct State {
  Event ready;
  int value;
};

Task wait_for_value(State* state) {
  co_await state->ready;
  state->value = 42;
  co_return;
}

int main() {
  State state = {};
  Task task = wait_for_value(&state);
  task.handle.resume();

  if (task.handle.done() || !state.ready.waiter) {
    return 1;
  }

  state.ready.signal();
  if (state.value != 42) {
    return 3;
  }
  if (!task.handle.done()) {
    return 2;
  }
  if (state.ready.waiter) {
    return 4;
  }

  task.handle.destroy();
  return 0;
}
