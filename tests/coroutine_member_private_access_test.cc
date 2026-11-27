// Regression: suspending coroutine member functions must retain access to
// private members when the resume function is synthesized.
#include <coroutine>

struct Task {
  struct promise_type {
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };

  std::coroutine_handle<promise_type> handle;

  explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;
  Task(Task&& o) noexcept : handle(o.handle) { o.handle = {}; }
  ~Task() {
    if (handle) {
      handle.destroy();
    }
  }
};

struct Server {
  Task run() {
    helper();
    ready_ = 1;
    co_return;
  }

 private:
  int ready_ = 0;
  void helper() { ready_ = 2; }
};

int main() {
  Server server;
  Task task = server.run();
  (void)task;
  return 0;
}
