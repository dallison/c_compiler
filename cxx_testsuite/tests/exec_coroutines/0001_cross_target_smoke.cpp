// RUN: -std=c++20

struct Task;
typedef struct Task Task;
struct Promise;

struct CoroutineFrame {
  int state;
  int done;
  void (*resume)(void* handle);
  void (*destroy)(void* handle);
};

void* last_frame_handle;
int last_resume_value;
int final_suspend_count;
int final_resume_count;
int final_dtor_count;
int operator_new_count;
int operator_delete_count;
int immovable_ctor_count;
int immovable_dtor_count;
unsigned long coroutine_frame_storage[128];
int coroutine_frame_storage_index;

void* operator new(unsigned long size) {
  int words =
      (int)((size + sizeof(unsigned long) - 1) / sizeof(unsigned long));
  void* result = &coroutine_frame_storage[coroutine_frame_storage_index];
  coroutine_frame_storage_index = coroutine_frame_storage_index + words;
  operator_new_count = operator_new_count + 1;
  return result;
}

void operator delete(void* ptr) {
  (void)ptr;
  operator_delete_count = operator_delete_count + 1;
}

struct SuspendNever {
  bool await_ready(void) {
    return true;
  }
  void await_suspend(void* handle) {
    (void)handle;
  }
  void await_resume(void) {
  }
};

struct SuspendOnce {
  int value;
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    last_frame_handle = handle;
  }
  int await_resume(void) {
    return value + 1;
  }
};

struct Immovable {
  int value;
  Immovable();
  Immovable(const Immovable& other) = delete;
  Immovable(Immovable&& other) = delete;
  ~Immovable();
};

Immovable::Immovable() {
  immovable_ctor_count = immovable_ctor_count + 1;
  value = 7;
}

Immovable::~Immovable() {
  immovable_dtor_count = immovable_dtor_count + 1;
}

struct FinalSuspendAlways {
  ~FinalSuspendAlways();
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    final_suspend_count = final_suspend_count + 1;
    last_frame_handle = handle;
  }
  void await_resume(void) {
    final_resume_count = final_resume_count + 1;
  }
};

FinalSuspendAlways::~FinalSuspendAlways() {
  final_dtor_count = final_dtor_count + 1;
}

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
  FinalSuspendAlways final_suspend(void) {
    FinalSuspendAlways value = {};
    return value;
  }
  void return_value(int result) {
    value = result;
  }
  void unhandled_exception(void) {
  }
};

Task coroutine_smoke(int input) {
  int kept = input + 2;
  int value = co_await SuspendOnce{5};
  co_return kept + value;
}

Task coroutine_immovable_local(void) {
  Immovable kept;
  int value = co_await SuspendOnce{5};
  co_return kept.value + value;
}

int frame_state(void* handle) {
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  return frame->state;
}

bool frame_done(void* handle) {
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  return frame->done != 0;
}

void resume_coroutine(void* handle) {
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  frame->resume(handle);
  unsigned long prefix_size = 2 * sizeof(int) + 2 * sizeof(void*);
  Promise* promise = (Promise*)((char*)handle + prefix_size);
  last_resume_value = promise->value;
}

void destroy_coroutine(void* handle) {
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  frame->destroy(handle);
}

int main(void) {
  Task started = coroutine_smoke(20);
  if (started.value != 0 || started.handle == 0 ||
      started.handle != last_frame_handle || frame_state(started.handle) != 1 ||
      frame_done(started.handle)) {
    return 1;
  }
  if (operator_new_count != 1 || operator_delete_count != 0) {
    return 2;
  }

  resume_coroutine(started.handle);
  CoroutineFrame* frame = (CoroutineFrame*)started.handle;
  if (last_resume_value != 28 || frame_state(started.handle) != 0 ||
      !frame_done(started.handle) || frame->resume != 0 ||
      frame->destroy == 0) {
    return 3;
  }
  if (final_suspend_count != 1 || final_resume_count != 0 ||
      final_dtor_count != 0) {
    return 4;
  }

  destroy_coroutine(started.handle);
  if (final_dtor_count != 1 || operator_delete_count != 1 ||
      frame->destroy != 0) {
    return 5;
  }

  Task immovable = coroutine_immovable_local();
  if (immovable.value != 0 || immovable.handle == 0 ||
      frame_state(immovable.handle) != 1 || immovable_ctor_count != 1 ||
      immovable_dtor_count != 0) {
    return 6;
  }
  resume_coroutine(immovable.handle);
  if (last_resume_value != 13 || !frame_done(immovable.handle) ||
      immovable_dtor_count != 1) {
    return 7;
  }
  destroy_coroutine(immovable.handle);
  if (immovable_dtor_count != 1 || final_suspend_count != 2 ||
      final_dtor_count != 2 || operator_new_count != 2 ||
      operator_delete_count != 2) {
    return 8;
  }
  return 0;
}
