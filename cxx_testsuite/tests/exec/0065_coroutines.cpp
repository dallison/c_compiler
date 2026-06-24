// RUN: -std=c++20

struct SuspendNever {
  bool await_ready(void) {
    return true;
  }
  void await_suspend(int handle) {
    (void)handle;
  }
  void await_resume(void) {
  }
};

struct ReadyInt {
  int value;
  bool await_ready(void) {
    return true;
  }
  void await_suspend(int handle) {
    (void)handle;
  }
  int await_resume(void) {
    return value;
  }
};

void* last_frame_handle;

struct SuspendOnce {
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    last_frame_handle = handle;
  }
  int await_resume(void) {
    return 58;
  }
};

struct Task;
typedef struct Task Task;
struct Promise;

struct CoroutineFrame {
  int state;
  int done;
  Task (*resume)(void);
  void (*destroy)(void);
};

int last_resume_value;
int final_suspend_count;
int suspend_twice_start_count;
int yield_twice_start_count;

struct SuspendValue {
  int value;
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    last_frame_handle = handle;
  }
  int await_resume(void) {
    return value;
  }
};

struct Task {
  using promise_type = Promise;
  int value;
  void* handle;
};

struct Promise {
  int value;
  Task get_return_object(void) {
    Task task = {value, last_frame_handle};
    return task;
  }

  SuspendNever initial_suspend(void) {
    SuspendNever value = {};
    return value;
  }

  SuspendNever final_suspend(void) {
    final_suspend_count = final_suspend_count + 1;
    SuspendNever value = {};
    return value;
  }

  void return_value(int result) {
    value = result;
  }

  void return_void(void) {
    value = 0;
  }

  SuspendValue yield_value(int result) {
    value = result;
    SuspendValue awaiter = {result};
    return awaiter;
  }

  void unhandled_exception(void) {
  }
};

Task coroutine_value(void) {
  co_return 23;
}

Task coroutine_void_value(void) {
  co_return;
}

Task coroutine_await_value(void) {
  ReadyInt awaiter = {37};
  int value = co_await awaiter;
  co_return value + 5;
}

Task coroutine_suspend_once(void) {
  SuspendOnce awaiter = {};
  int value = co_await awaiter;
  co_return value + 2;
}

Task coroutine_suspend_twice(void) {
  suspend_twice_start_count = suspend_twice_start_count + 1;
  SuspendValue first = {11};
  int first_value = co_await first;
  SuspendValue second = {23};
  int second_value = co_await second;
  (void)first_value;
  co_return second_value + 15;
}

Task coroutine_yield_twice(void) {
  yield_twice_start_count = yield_twice_start_count + 1;
  co_yield 101;
  co_yield 202;
  co_return 303;
}

int frame_state(void* handle) {
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  return frame->state;
}

int frame_done(void* handle) {
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  return frame->done;
}

void resume_coroutine(void* handle) {
  last_resume_value = -1;
  if (handle == 0 || frame_done(handle) || frame_state(handle) == 0) {
    return;
  }
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  if (frame->resume == 0) {
    return;
  }
  Task task = frame->resume();
  last_resume_value = task.value;
}

void resume_yield_coroutine(void* handle) {
  last_resume_value = -1;
  if (handle == 0 || frame_done(handle) || frame_state(handle) == 0) {
    return;
  }
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  if (frame->resume == 0) {
    return;
  }
  Task task = frame->resume();
  last_resume_value = task.value;
}

void destroy_coroutine(void* handle) {
  if (handle == 0) {
    return;
  }
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  if (frame->destroy != 0) {
    frame->destroy();
  }
}

int main(void) {
  Task value = coroutine_value();
  if (value.value != 23) {
    return 1;
  }

  Task empty = coroutine_void_value();
  if (empty.value != 0) {
    return 2;
  }

  Task awaited = coroutine_await_value();
  if (awaited.value != 42) {
    return 3;
  }

  Task started = coroutine_suspend_once();
  if (started.value != 0) {
    return 4;
  }

  resume_coroutine(started.handle);
  if (last_resume_value != 60) {
    return 5;
  }

  Task twice_started = coroutine_suspend_twice();
  if (twice_started.value != 0 || twice_started.handle == 0 ||
      frame_state(twice_started.handle) != 1 ||
      frame_done(twice_started.handle)) {
    return 6;
  }
  if (suspend_twice_start_count != 1) {
    return 19;
  }

  resume_coroutine(twice_started.handle);
  if (last_resume_value != 0) {
    return 7;
  }
  if (last_frame_handle == 0) {
    return 9;
  }
  if (frame_state(last_frame_handle) != 2 || frame_done(last_frame_handle)) {
    return 10;
  }

  resume_coroutine(last_frame_handle);
  if (last_resume_value != 38) {
    return 8;
  }
  if (last_frame_handle == 0 || frame_state(last_frame_handle) != 0 ||
      !frame_done(last_frame_handle)) {
    return 11;
  }
  if (suspend_twice_start_count != 1) {
    return 20;
  }
  if (final_suspend_count != 5) {
    return 12;
  }
  CoroutineFrame* completed_twice_frame = (CoroutineFrame*)last_frame_handle;
  if (completed_twice_frame->resume != 0 ||
      completed_twice_frame->destroy == 0) {
    return 28;
  }

  resume_coroutine(last_frame_handle);
  if (last_resume_value != -1 || final_suspend_count != 5) {
    return 13;
  }
  destroy_coroutine(last_frame_handle);
  if (frame_state(last_frame_handle) != 0 || !frame_done(last_frame_handle) ||
      completed_twice_frame->resume != 0 ||
      completed_twice_frame->destroy != 0 || final_suspend_count != 5) {
    return 29;
  }

  Task destroy_started = coroutine_suspend_twice();
  if (destroy_started.handle == 0 || frame_state(destroy_started.handle) != 1 ||
      frame_done(destroy_started.handle)) {
    return 23;
  }
  if (suspend_twice_start_count != 2) {
    return 24;
  }

  destroy_coroutine(destroy_started.handle);
  CoroutineFrame* destroyed_frame = (CoroutineFrame*)destroy_started.handle;
  if (frame_state(destroy_started.handle) != 0 ||
      !frame_done(destroy_started.handle) || destroyed_frame->resume != 0 ||
      destroyed_frame->destroy != 0) {
    return 25;
  }
  resume_coroutine(destroy_started.handle);
  if (last_resume_value != -1 || suspend_twice_start_count != 2 ||
      final_suspend_count != 5) {
    return 26;
  }
  destroy_coroutine(destroy_started.handle);
  if (last_resume_value != -1 || suspend_twice_start_count != 2 ||
      final_suspend_count != 5) {
    return 27;
  }

  Task yielded = coroutine_yield_twice();
  if (yielded.value != 101 || yielded.handle == 0 ||
      frame_state(yielded.handle) != 1 || frame_done(yielded.handle)) {
    return 14;
  }
  if (yield_twice_start_count != 1) {
    return 21;
  }

  resume_yield_coroutine(yielded.handle);
  if (last_resume_value != 202 || last_frame_handle == 0 ||
      frame_state(last_frame_handle) != 2 || frame_done(last_frame_handle)) {
    return 15;
  }

  resume_yield_coroutine(last_frame_handle);
  if (last_resume_value != 303 || last_frame_handle == 0 ||
      frame_state(last_frame_handle) != 0 || !frame_done(last_frame_handle)) {
    return 16;
  }
  CoroutineFrame* completed_yield_frame = (CoroutineFrame*)last_frame_handle;
  if (completed_yield_frame->resume != 0 ||
      completed_yield_frame->destroy == 0) {
    return 30;
  }
  if (yield_twice_start_count != 1) {
    return 22;
  }
  if (final_suspend_count != 6) {
    return 17;
  }

  resume_yield_coroutine(last_frame_handle);
  if (last_resume_value != -1 || final_suspend_count != 6) {
    return 18;
  }

  return 0;
}
