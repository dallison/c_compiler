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

struct Mailbox {
  int value;
  bool ready;
  bool acknowledged;
};

Task sender(Mailbox* mailbox) {
  mailbox->value = 42;
  mailbox->ready = true;

  // Give the receiver a chance to consume the value.
  co_await std::suspend_always{};

  while (!mailbox->acknowledged) {
    co_await std::suspend_always{};
  }
  co_return;
}

Task receiver(Mailbox* mailbox) {
  while (!mailbox->ready) {
    co_await std::suspend_always{};
  }

  std::cout << "received: " << mailbox->value << '\n';
  mailbox->acknowledged = true;
  co_return;
}

int main() {
  Mailbox mailbox = {};
  Task receive = receiver(&mailbox);
  Task send = sender(&mailbox);

  // Start the receiver first so it waits for the sender.
  while (!send.done() || !receive.done()) {
    if (!receive.done()) {
      receive.resume();
    }
    if (!send.done()) {
      send.resume();
    }
  }

  receive.destroy();
  send.destroy();
  return 0;
}
