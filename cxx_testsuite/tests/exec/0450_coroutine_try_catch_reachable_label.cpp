// RUN: -std=c++20
// EXPECT_EXIT: 0
// The coroutine ramp function keeps unreachable blocks that carry an exception
// table label.  Those blocks are still emitted, so the blocks they branch to
// have to survive unreachable-block removal too, or the emitted branch refers
// to a label that no longer exists.

#include <coroutine>

static int thrower_calls;
static int returned_value;
static std::coroutine_handle<> captured_handle;

static void thrower(void) {
  thrower_calls++;
  throw 19;
}

struct CaptureSuspend {
  bool await_ready(void) const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    captured_handle = handle;
  }

  void await_resume(void) const noexcept {
  }
};

struct Task {
  struct promise_type {
    Task get_return_object(void) {
      Task task = {};
      return task;
    }

    std::suspend_never initial_suspend(void) {
      return {};
    }

    std::suspend_never final_suspend(void) noexcept {
      return {};
    }

    void return_value(int result) {
      returned_value = result;
    }

    void unhandled_exception(void) {
    }
  };
};

static Task nested_try_catches_throw(void) {
  co_await CaptureSuspend{};
  try {
    thrower();
  } catch (int caught) {
    co_return caught + 300;
  }
  co_return 1;
}

int main(void) {
  nested_try_catches_throw();
  if (thrower_calls != 0 || returned_value != 0) {
    return 1;
  }
  if (!captured_handle) {
    return 2;
  }
  captured_handle.resume();
  if (thrower_calls != 1) {
    return 3;
  }
  if (returned_value != 319) {
    return 4;
  }
  return 0;
}
