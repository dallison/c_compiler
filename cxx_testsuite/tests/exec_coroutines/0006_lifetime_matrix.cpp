// RUN: -std=c++20 -O2
// EXPECT_EXIT: 0

#include <coroutine>

struct Promise;

struct Task {
  using promise_type = Promise;
  std::coroutine_handle<Promise> handle;
};

int tracked_ctor_count;
int tracked_dtor_count;
int parameter_dtor_count;
int unhandled_count;

struct Tracked {
  int value;

  Tracked(int initial) : value(initial) {
    ++tracked_ctor_count;
  }

  Tracked(const Tracked& other) : value(other.value) {
    ++tracked_ctor_count;
  }

  Tracked(Tracked&& other) : value(other.value) {
    other.value = -1;
    ++tracked_ctor_count;
  }

  ~Tracked() {
    ++tracked_dtor_count;
    if (value == 9) {
      ++parameter_dtor_count;
    }
  }
};

struct SuspendOnce {
  bool await_ready() const noexcept {
    return false;
  }

  void await_suspend(std::coroutine_handle<>) const noexcept {
  }

  void await_resume() const noexcept {
  }
};

struct Promise {
  int value;

  Task get_return_object() {
    return {std::coroutine_handle<Promise>::from_promise(*this)};
  }

  std::suspend_always initial_suspend() noexcept {
    return {};
  }

  std::suspend_always final_suspend() noexcept {
    return {};
  }

  void return_value(int result) noexcept {
    value = result;
  }

  void unhandled_exception() noexcept {
    ++unhandled_count;
  }
};

Task initial_suspend_parameter(int value) {
  co_return value + 1;
}

Task reference_parameter(int& value) {
  co_await SuspendOnce{};
  co_return value + 1;
}

Task break_destroys_local() {
  int before = tracked_dtor_count;
  for (;;) {
    Tracked local(1);
    co_await SuspendOnce{};
    break;
  }
  co_return tracked_dtor_count - before;
}

Task continue_destroys_local() {
  int before = tracked_dtor_count;
  for (int iteration = 0; iteration != 2; ++iteration) {
    Tracked local(iteration);
    co_await SuspendOnce{};
    if (iteration == 0) {
      continue;
    }
  }
  co_return tracked_dtor_count - before;
}

Task class_for_initializer() {
  int before = tracked_dtor_count;
  for (Tracked state(0); state.value != 2; ++state.value) {
    co_await SuspendOnce{};
  }
  co_return tracked_dtor_count - before;
}

int range_values[2] = {2, 4};

Task range_for_lifetime() {
  int sum = 0;
  for (int value : range_values) {
    co_await SuspendOnce{};
    sum += value;
  }
  co_return sum;
}

struct Owner {
  int value;

  Task member_coroutine() {
    co_await SuspendOnce{};
    value += 2;
    co_return value;
  }
};

#if defined(__cpp_exceptions)
Task parameter_survives_exception(Tracked parameter) {
  co_await SuspendOnce{};
  throw parameter.value;
  co_return 0;
}
#endif

Task class_parameter(Tracked parameter) {
  co_await SuspendOnce{};
  co_return parameter.value;
}

Task unused_class_parameter(Tracked parameter) {
  co_await SuspendOnce{};
  co_return 0;
}

int main() {
  Task initial = initial_suspend_parameter(41);
  initial.handle.resume();
  if (!initial.handle.done() || initial.handle.promise().value != 42) return 1;
  initial.handle.destroy();

  int referenced = 40;
  Task reference = reference_parameter(referenced);
  reference.handle.resume();
  referenced = 41;
  reference.handle.resume();
  if (!reference.handle.done() || reference.handle.promise().value != 42) {
    return 2;
  }
  reference.handle.destroy();

  int before = tracked_dtor_count;
  Task broken = break_destroys_local();
  broken.handle.resume();
  if (tracked_dtor_count != before) return 3;
  broken.handle.resume();
  if (!broken.handle.done() || broken.handle.promise().value != 1 ||
      tracked_dtor_count != before + 1) {
    return 4;
  }
  broken.handle.destroy();
  if (tracked_dtor_count != before + 1) return 5;

  before = tracked_dtor_count;
  Task continued = continue_destroys_local();
  continued.handle.resume();
  continued.handle.resume();
  if (tracked_dtor_count != before + 1 || continued.handle.done()) return 6;
  continued.handle.resume();
  if (!continued.handle.done() || continued.handle.promise().value != 2 ||
      tracked_dtor_count != before + 2) {
    return 7;
  }
  continued.handle.destroy();
  if (tracked_dtor_count != before + 2) return 8;

  before = tracked_dtor_count;
  Task initialized = class_for_initializer();
  initialized.handle.resume();
  initialized.handle.resume();
  initialized.handle.resume();
  if (!initialized.handle.done() || initialized.handle.promise().value != 1 ||
      tracked_dtor_count != before + 1) {
    return 9;
  }
  initialized.handle.destroy();
  if (tracked_dtor_count != before + 1) return 10;

  Task ranged = range_for_lifetime();
  ranged.handle.resume();
  ranged.handle.resume();
  ranged.handle.resume();
  if (!ranged.handle.done() || ranged.handle.promise().value != 6) return 11;
  ranged.handle.destroy();

  Owner owner = {40};
  Task member = owner.member_coroutine();
  member.handle.resume();
  member.handle.resume();
  if (!member.handle.done() || member.handle.promise().value != 42 ||
      owner.value != 42) {
    return 12;
  }
  member.handle.destroy();

  Task class_kept = class_parameter(Tracked(7));
  int after_class_parameter_copy = tracked_dtor_count;
  class_kept.handle.resume();
  class_kept.handle.resume();
  if (!class_kept.handle.done() || class_kept.handle.promise().value != 7 ||
      tracked_dtor_count != after_class_parameter_copy) {
    return 18;
  }
  class_kept.handle.destroy();
  if (tracked_dtor_count != after_class_parameter_copy + 1) return 19;

  Task class_destroyed_early = class_parameter(Tracked(8));
  int before_early_destroy = tracked_dtor_count;
  class_destroyed_early.handle.resume();
  class_destroyed_early.handle.destroy();
  if (tracked_dtor_count != before_early_destroy + 1) return 20;

  Task unused_class = unused_class_parameter(Tracked(6));
  int before_unused_destroy = tracked_dtor_count;
  unused_class.handle.resume();
  unused_class.handle.destroy();
  if (tracked_dtor_count != before_unused_destroy + 1) return 21;

#if defined(__cpp_exceptions)
  before = tracked_dtor_count;
  Task exceptional = parameter_survives_exception(Tracked(9));
  int after_parameter_copy = tracked_dtor_count;
  int parameter_dtors_before_resume = parameter_dtor_count;
  exceptional.handle.resume();
  if (tracked_dtor_count != after_parameter_copy) return 15;
  exceptional.handle.resume();
  if (unhandled_count != 1) return 17;
  if (parameter_dtor_count != parameter_dtors_before_resume) return 13;
  exceptional.handle.destroy();
  if (parameter_dtor_count != parameter_dtors_before_resume + 1 ||
      tracked_dtor_count < before + 1) {
    return 14;
  }
#endif

  return 0;
}
