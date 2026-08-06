// RUN: -std=c++20 -O0
// EXPECT_EXIT: 0

#include <coroutine>

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
  int received_value;
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
  mailbox->received_value = mailbox->value;
  mailbox->acknowledged.signal();
  co_return;
}

int main() {
  Mailbox mailbox = {};
  Task receive = receiver(&mailbox);
  Task send = sender(&mailbox);

  receive.resume();
  if (receive.done() || !mailbox.ready.waiter) {
    return 1;
  }

  send.resume();
  bool complete = send.done() && receive.done();
  bool exchanged = mailbox.received_value == 42 && mailbox.ready.signaled &&
                   mailbox.acknowledged.signaled && !mailbox.ready.waiter &&
                   !mailbox.acknowledged.waiter;

  receive.destroy();
  send.destroy();
  return complete && exchanged ? 0 : 2;
}
