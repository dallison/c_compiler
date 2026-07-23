// RUN: -std=c++20

#include <coroutine>

static int lazy_stage;
static int lazy_error;
static int lazy_return_object_calls;

struct LazyPromise;

struct LazyTask {
  using promise_type = LazyPromise;
  std::coroutine_handle<LazyPromise> handle;
};

struct LazyPromise {
  LazyTask get_return_object() {
    ++lazy_return_object_calls;
    if (lazy_stage != 0) {
      lazy_error = 1;
    }
    lazy_stage = 1;
    return {std::coroutine_handle<LazyPromise>::from_promise(*this)};
  }

  std::suspend_always initial_suspend() noexcept {
    if (lazy_stage != 1) {
      lazy_error = 2;
    }
    lazy_stage = 2;
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

LazyTask make_lazy() {
  co_return;
}

static int eager_stage;
static int eager_error;
static int eager_return_object_calls;

struct EagerPromise;

struct EagerTask {
  using promise_type = EagerPromise;
  int value;
};

struct EagerPromise {
  EagerTask get_return_object() {
    ++eager_return_object_calls;
    if (eager_stage != 0) {
      eager_error = 1;
    }
    eager_stage = 1;
    return {42};
  }

  std::suspend_never initial_suspend() noexcept {
    if (eager_stage != 1) {
      eager_error = 2;
    }
    eager_stage = 2;
    return {};
  }

  std::suspend_never final_suspend() noexcept {
    return {};
  }

  void return_void() noexcept {
  }

  void unhandled_exception() noexcept {
  }
};

EagerTask make_eager() {
  co_return;
}

int main() {
  LazyTask lazy = make_lazy();
  if (lazy_error != 0 || lazy_stage != 2 ||
      lazy_return_object_calls != 1 || !lazy.handle ||
      lazy.handle.done()) {
    return 1;
  }
  lazy.handle.resume();
  if (!lazy.handle.done() || lazy_return_object_calls != 1) {
    return 2;
  }
  lazy.handle.destroy();

  EagerTask eager = make_eager();
  if (eager_error != 0 || eager_stage != 2 ||
      eager_return_object_calls != 1 || eager.value != 42) {
    return 3;
  }
  return 0;
}
