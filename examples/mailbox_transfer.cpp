// Mailbox transfer between two coroutines.
//
// reader() co_awaits mailbox::receive(), a sender. That connects a receiver
// and suspends. writer() posts an int; post() completes the parked receiver
// with set_value, and that resumes reader().

#include <coroutine>
#include <exception>
#include <execution>
#include <utility>

template <class T>
struct mailbox;

template <class T>
struct mailbox_receiver {
  using receiver_concept = std::execution::receiver_t;
  mailbox<T>* box;

  void set_value(T value) {
    box->delivered = static_cast<T&&>(value);
    box->has_result = true;
    if (box->armed && box->waiter) {
      box->waiter.resume();
    }
  }
  void set_error(std::exception_ptr) {}
  void set_stopped() {}
  std::execution::env<> get_env() const { return {}; }
};

template <class T>
struct mailbox_op {
  using operation_state_concept = std::execution::operation_state_t;
  mailbox<T>* box;
  mailbox_receiver<T> rcvr;

  mailbox_op(mailbox<T>* owner, mailbox_receiver<T> receiver)
      : box(owner), rcvr(receiver) {}

  void start() noexcept {
    if (box->full) {
      box->full = false;
      T value = static_cast<T&&>(box->slot);
      std::execution::set_value(rcvr, static_cast<T&&>(value));
      return;
    }
    box->waiting = this;
  }
};

template <class T>
struct mailbox {
  bool full;
  bool has_result;
  bool armed;
  T slot;
  T delivered;
  std::coroutine_handle<> waiter;
  mailbox_op<T>* waiting;
  mailbox_op<T>* owned;

  mailbox()
      : full(false),
        has_result(false),
        armed(false),
        slot(),
        delivered(),
        waiter(),
        waiting(nullptr),
        owned(nullptr) {}

  ~mailbox() { delete owned; }

  struct receive_sender {
    using sender_concept = std::execution::sender_t;
    mailbox* box;

    template <class Env>
    using completions =
        std::execution::completion_signatures<std::execution::set_value_t(T)>;

    std::execution::env<> get_env() const noexcept { return {}; }

    mailbox_op<T> connect(mailbox_receiver<T> rcvr) {
      return mailbox_op<T>(box, static_cast<mailbox_receiver<T>&&>(rcvr));
    }
  };

  receive_sender receive() { return receive_sender{this}; }

  void post(T value) {
    if (waiting != nullptr) {
      mailbox_op<T>* op = waiting;
      waiting = nullptr;
      std::execution::set_value(op->rcvr, static_cast<T&&>(value));
      return;
    }
    slot = static_cast<T&&>(value);
    full = true;
  }
};

struct receive_awaiter {
  mailbox<int>* box;

  bool await_ready() noexcept { return false; }

  bool await_suspend(std::coroutine_handle<> handle) {
    box->waiter = handle;
    mailbox_op<int> connected = std::execution::connect(
        box->receive(), mailbox_receiver<int>{box});
    box->owned =
        new mailbox_op<int>(static_cast<mailbox_op<int>&&>(connected));
    std::execution::start(*box->owned);
    box->armed = true;
    return !box->has_result;
  }

  int await_resume() { return box->delivered; }
};

receive_awaiter operator co_await(mailbox<int>::receive_sender sndr) {
  return receive_awaiter{sndr.box};
}

struct reading {
  struct promise_type {
    int result;
    promise_type() : result(0) {}
    reading get_return_object() {
      return reading{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(int value) { result = value; }
    void unhandled_exception() {}
  };

  std::coroutine_handle<promise_type> coro;
};

struct writing {
  struct promise_type {
    writing get_return_object() {
      return writing{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };

  std::coroutine_handle<promise_type> coro;
};

reading reader(mailbox<int>& box) {
  int message = co_await box.receive();
  co_return message;
}

writing writer(mailbox<int>& box, int message) {
  box.post(message);
  co_return;
}

int main() {
  mailbox<int> box;
  reading incoming = reader(box);
  incoming.coro.resume();
  if (incoming.coro.done()) {
    return 1;
  }

  writing outgoing = writer(box, 42);
  outgoing.coro.resume();

  int received = incoming.coro.promise().result;
  bool finished = incoming.coro.done() && outgoing.coro.done();
  incoming.coro.destroy();
  outgoing.coro.destroy();
  if (!finished || received != 42) {
    return 2;
  }
  return 0;
}
