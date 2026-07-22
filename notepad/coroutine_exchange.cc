#include <coroutine>
#include <iostream>

struct Promise;

struct Task {
  using promise_type = Promise;
  using handle_type = std::coroutine_handle<Promise>;

  handle_type handle;

  bool done() const {
    return handle.done();
  }

  void resume() {
    handle.resume();
  }

  void destroy() {
    handle.destroy();
  }
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

struct OneShotEvent {
  struct Awaiter {
    OneShotEvent* event;

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

struct Mailbox {
  int value;
  OneShotEvent ready;
  OneShotEvent acknowledged;
};

Task sender(Mailbox* mailbox) {
  mailbox->value = 42;
  mailbox->ready.signal();
  co_await mailbox->acknowledged;
  co_return;
}

Task receiver(Mailbox* mailbox) {
  co_await mailbox->ready;

  std::cout << "received: " << mailbox->value << '\n';
  mailbox->acknowledged.signal();
  co_return;
}

int main() {
  Mailbox mailbox = {};
  Task receive = receiver(&mailbox);
  Task send = sender(&mailbox);

  // The receiver registers itself with ready; signaling ready then drives the
  // exchange until both coroutines reach their final suspend points.
  receive.resume();
  send.resume();
  bool complete = send.done() && receive.done();

  receive.destroy();
  send.destroy();
  return complete ? 0 : 1;
}
