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
  Task (*resume)(void* handle);
  void (*destroy)(void* handle);
};

int last_resume_value;
int final_suspend_count;
int initial_suspend_count;
int initial_always_suspend_count;
int initial_always_resume_count;
int initial_suspend_body_count;
int suspend_twice_start_count;
int suspend_unique_start_count;
int suspend_unique_next_base;
int yield_twice_start_count;
int global_coroutine_operator_new_count;
int global_coroutine_operator_delete_count;
int promise_coroutine_operator_new_count;
int promise_coroutine_operator_delete_count;
int lifetime_promise_ctor_count;
int lifetime_promise_dtor_count;
int tracked_awaiter_ctor_count;
int tracked_awaiter_copy_count;
int tracked_awaiter_dtor_count;
int persisted_local_ctor_count;
int persisted_local_copy_count;
int persisted_local_dtor_count;
int persisted_param_ctor_count;
int persisted_param_copy_count;
int persisted_param_move_count;
int persisted_param_dtor_count;
int member_coawait_temp_dtor_count;
int free_coawait_temp_dtor_count;
int bool_await_suspend_count;
unsigned long coroutine_frame_storage[512];
int coroutine_frame_storage_index;

void* operator new(unsigned long size) {
  int words = (int)((size + 7) / 8);
  void* result = &coroutine_frame_storage[coroutine_frame_storage_index];
  coroutine_frame_storage_index = coroutine_frame_storage_index + words;
  global_coroutine_operator_new_count =
      global_coroutine_operator_new_count + 1;
  return result;
}

void operator delete(void* ptr) {
  (void)ptr;
  global_coroutine_operator_delete_count =
      global_coroutine_operator_delete_count + 1;
}

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

struct BoolSuspendValue {
  int value;
  bool should_suspend;
  bool await_ready(void) {
    return false;
  }
  bool await_suspend(void* handle) {
    last_frame_handle = handle;
    bool_await_suspend_count = bool_await_suspend_count + 1;
    return should_suspend;
  }
  int await_resume(void) {
    return value;
  }
};

struct DirectSuspendValue {
  int value;
  DirectSuspendValue(int input);
  DirectSuspendValue(const DirectSuspendValue& other);
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

DirectSuspendValue::DirectSuspendValue(int input) {
  value = input;
}

DirectSuspendValue::DirectSuspendValue(const DirectSuspendValue& other) {
  value = other.value;
}

struct MemberCoAwaitValue {
  int value;
  MemberCoAwaitValue(int input);
  MemberCoAwaitValue(const MemberCoAwaitValue& other);
  SuspendValue operator co_await(void);
};

MemberCoAwaitValue::MemberCoAwaitValue(int input) {
  value = input;
}

MemberCoAwaitValue::MemberCoAwaitValue(const MemberCoAwaitValue& other) {
  value = other.value;
}

SuspendValue MemberCoAwaitValue::operator co_await(void) {
  SuspendValue awaiter = {value + 6};
  return awaiter;
}

struct MemberCoAwaitTrackedTemp {
  int value;
  MemberCoAwaitTrackedTemp(int input);
  MemberCoAwaitTrackedTemp(const MemberCoAwaitTrackedTemp& other);
  ~MemberCoAwaitTrackedTemp();
  SuspendValue operator co_await(void);
};

MemberCoAwaitTrackedTemp::MemberCoAwaitTrackedTemp(int input) {
  value = input;
}

MemberCoAwaitTrackedTemp::MemberCoAwaitTrackedTemp(
    const MemberCoAwaitTrackedTemp& other) {
  value = other.value;
}

MemberCoAwaitTrackedTemp::~MemberCoAwaitTrackedTemp() {
  member_coawait_temp_dtor_count = member_coawait_temp_dtor_count + 1;
}

SuspendValue MemberCoAwaitTrackedTemp::operator co_await(void) {
  SuspendValue awaiter = {value + 6};
  return awaiter;
}

struct FreeCoAwaitValue {
  int value;
  FreeCoAwaitValue(int input);
  FreeCoAwaitValue(const FreeCoAwaitValue& other);
  ~FreeCoAwaitValue();
};

FreeCoAwaitValue::FreeCoAwaitValue(int input) {
  value = input;
}

FreeCoAwaitValue::FreeCoAwaitValue(const FreeCoAwaitValue& other) {
  value = other.value;
}

FreeCoAwaitValue::~FreeCoAwaitValue() {
  free_coawait_temp_dtor_count = free_coawait_temp_dtor_count + 1;
}

SuspendValue operator co_await(FreeCoAwaitValue& awaitable) {
  SuspendValue awaiter = {awaitable.value + 7};
  return awaiter;
}

SuspendValue operator co_await(FreeCoAwaitValue&& awaitable) {
  SuspendValue awaiter = {awaitable.value + 7};
  return awaiter;
}

struct OtherFreeCoAwaitValue {
  int value;
  OtherFreeCoAwaitValue(int input);
  OtherFreeCoAwaitValue(const OtherFreeCoAwaitValue& other);
};

OtherFreeCoAwaitValue::OtherFreeCoAwaitValue(int input) {
  value = input;
}

OtherFreeCoAwaitValue::OtherFreeCoAwaitValue(
    const OtherFreeCoAwaitValue& other) {
  value = other.value;
}

SuspendValue operator co_await(OtherFreeCoAwaitValue& awaitable) {
  SuspendValue awaiter = {awaitable.value + 8};
  return awaiter;
}

SuspendValue operator co_await(OtherFreeCoAwaitValue&& awaitable) {
  SuspendValue awaiter = {awaitable.value + 8};
  return awaiter;
}

struct CvRefFreeCoAwaitValue {
  int value;
  CvRefFreeCoAwaitValue(int input);
  CvRefFreeCoAwaitValue(const CvRefFreeCoAwaitValue& other);
};

CvRefFreeCoAwaitValue::CvRefFreeCoAwaitValue(int input) {
  value = input;
}

CvRefFreeCoAwaitValue::CvRefFreeCoAwaitValue(
    const CvRefFreeCoAwaitValue& other) {
  value = other.value;
}

SuspendValue operator co_await(CvRefFreeCoAwaitValue& awaitable) {
  SuspendValue awaiter = {awaitable.value + 10};
  return awaiter;
}

SuspendValue operator co_await(const CvRefFreeCoAwaitValue& awaitable) {
  SuspendValue awaiter = {awaitable.value + 20};
  return awaiter;
}

SuspendValue operator co_await(CvRefFreeCoAwaitValue&& awaitable) {
  SuspendValue awaiter = {awaitable.value + 30};
  return awaiter;
}

namespace adl_coawait {
struct Awaitable {
  int value;
  Awaitable(int input);
  Awaitable(const Awaitable& other);
};

Awaitable::Awaitable(int input) {
  value = input;
}

Awaitable::Awaitable(const Awaitable& other) {
  value = other.value;
}

SuspendValue operator co_await(Awaitable& awaitable) {
  SuspendValue awaiter = {awaitable.value + 40};
  return awaiter;
}

SuspendValue operator co_await(const Awaitable& awaitable) {
  SuspendValue awaiter = {awaitable.value + 50};
  return awaiter;
}

SuspendValue operator co_await(Awaitable&& awaitable) {
  SuspendValue awaiter = {awaitable.value + 60};
  return awaiter;
}

struct MemberPreferredAwaitable {
  int value;
  MemberPreferredAwaitable(int input);
  MemberPreferredAwaitable(const MemberPreferredAwaitable& other);
  SuspendValue operator co_await(void);
};

MemberPreferredAwaitable::MemberPreferredAwaitable(int input) {
  value = input;
}

MemberPreferredAwaitable::MemberPreferredAwaitable(
    const MemberPreferredAwaitable& other) {
  value = other.value;
}

SuspendValue MemberPreferredAwaitable::operator co_await(void) {
  SuspendValue awaiter = {value + 70};
  return awaiter;
}

SuspendValue operator co_await(MemberPreferredAwaitable& awaitable) {
  SuspendValue awaiter = {awaitable.value + 80};
  return awaiter;
}
}

struct TrackedAwaiter {
  int value;
  TrackedAwaiter(int input);
  TrackedAwaiter(const TrackedAwaiter& other);
  ~TrackedAwaiter();
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

TrackedAwaiter::TrackedAwaiter(int input) {
  tracked_awaiter_ctor_count = tracked_awaiter_ctor_count + 1;
  value = input;
}

TrackedAwaiter::TrackedAwaiter(const TrackedAwaiter& other) {
  tracked_awaiter_copy_count = tracked_awaiter_copy_count + 1;
  value = other.value;
}

TrackedAwaiter::~TrackedAwaiter() {
  tracked_awaiter_dtor_count = tracked_awaiter_dtor_count + 1;
}

struct PersistedParamBox {
  int value;
  PersistedParamBox(int input);
  PersistedParamBox(const PersistedParamBox& other);
  PersistedParamBox(PersistedParamBox&& other);
  ~PersistedParamBox();
};

PersistedParamBox::PersistedParamBox(int input) {
  persisted_param_ctor_count = persisted_param_ctor_count + 1;
  value = input;
}

PersistedParamBox::PersistedParamBox(const PersistedParamBox& other) {
  persisted_param_copy_count = persisted_param_copy_count + 1;
  value = other.value;
}

PersistedParamBox::PersistedParamBox(PersistedParamBox&& other) {
  persisted_param_move_count = persisted_param_move_count + 1;
  value = other.value + 100;
}

PersistedParamBox::~PersistedParamBox() {
  persisted_param_dtor_count = persisted_param_dtor_count + 1;
}

struct PersistedLocalBox {
  int value;
  PersistedLocalBox(int input);
  PersistedLocalBox(const PersistedLocalBox& other);
  ~PersistedLocalBox();
};

PersistedLocalBox::PersistedLocalBox(int input) {
  persisted_local_ctor_count = persisted_local_ctor_count + 1;
  value = input;
}

PersistedLocalBox::PersistedLocalBox(const PersistedLocalBox& other) {
  persisted_local_copy_count = persisted_local_copy_count + 1;
  value = other.value + 200;
}

PersistedLocalBox::~PersistedLocalBox() {
  persisted_local_dtor_count = persisted_local_dtor_count + 1;
}

struct Task {
  using promise_type = Promise;
  int value;
  void* handle;
};

struct Promise {
  int value;
  void* operator new(unsigned long size);
  void operator delete(void* ptr);

  Task get_return_object(void) {
    Task task = {value, last_frame_handle};
    return task;
  }

  SuspendNever initial_suspend(void) {
    initial_suspend_count = initial_suspend_count + 1;
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

void* Promise::operator new(unsigned long size) {
  int words = (int)((size + 7) / 8);
  void* result = &coroutine_frame_storage[coroutine_frame_storage_index];
  coroutine_frame_storage_index = coroutine_frame_storage_index + words;
  promise_coroutine_operator_new_count =
      promise_coroutine_operator_new_count + 1;
  return result;
}

void Promise::operator delete(void* ptr) {
  (void)ptr;
  promise_coroutine_operator_delete_count =
      promise_coroutine_operator_delete_count + 1;
}

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

Task coroutine_bool_await_suspend_continue(void) {
  BoolSuspendValue awaiter = {101, false};
  int value = co_await awaiter;
  co_return value + 4;
}

Task coroutine_bool_await_suspend_suspend(void) {
  BoolSuspendValue awaiter = {103, true};
  int value = co_await awaiter;
  co_return value + 4;
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

Task coroutine_suspend_twice_unique(void) {
  int base = suspend_unique_next_base;
  suspend_unique_next_base = suspend_unique_next_base + 100;
  suspend_unique_start_count = suspend_unique_start_count + 1;
  SuspendValue first = {base + 11};
  int first_value = co_await first;
  SuspendValue second = {first_value + 12};
  int second_value = co_await second;
  co_return second_value + 15;
}

SuspendValue make_suspend_value(int value) {
  SuspendValue awaiter = {value};
  return awaiter;
}

Task coroutine_direct_await_expression(void) {
  int value = co_await DirectSuspendValue{17};
  co_return value + 4;
}

Task coroutine_direct_await_aggregate_expression(void) {
  int value = co_await SuspendValue{23};
  co_return value + 4;
}

Task coroutine_direct_await_factory_expression(void) {
  int value = co_await make_suspend_value(25);
  co_return value + 4;
}

Task coroutine_member_operator_co_await_named(void) {
  MemberCoAwaitValue awaitable(31);
  int value = co_await awaitable;
  co_return value + 4;
}

Task coroutine_member_operator_co_await_temporary(void) {
  int value = co_await MemberCoAwaitTrackedTemp{41};
  co_return value + 4;
}

Task coroutine_free_operator_co_await_named(void) {
  FreeCoAwaitValue awaitable(51);
  int value = co_await awaitable;
  co_return value + 4;
}

Task coroutine_free_operator_co_await_temporary(void) {
  int value = co_await FreeCoAwaitValue{61};
  co_return value + 4;
}

Task coroutine_free_operator_co_await_second_overload(void) {
  int value = co_await OtherFreeCoAwaitValue{71};
  co_return value + 4;
}

Task coroutine_free_operator_co_await_cvref_lvalue(void) {
  CvRefFreeCoAwaitValue awaitable(81);
  int value = co_await awaitable;
  co_return value + 4;
}

Task coroutine_free_operator_co_await_cvref_const_lvalue(void) {
  const CvRefFreeCoAwaitValue awaitable(82);
  int value = co_await awaitable;
  co_return value + 4;
}

Task coroutine_free_operator_co_await_cvref_rvalue(void) {
  int value = co_await CvRefFreeCoAwaitValue{83};
  co_return value + 4;
}

Task coroutine_adl_operator_co_await_lvalue(void) {
  adl_coawait::Awaitable awaitable(91);
  int value = co_await awaitable;
  co_return value + 4;
}

Task coroutine_adl_operator_co_await_const_lvalue(void) {
  const adl_coawait::Awaitable awaitable(92);
  int value = co_await awaitable;
  co_return value + 4;
}

Task coroutine_adl_operator_co_await_rvalue(void) {
  int value = co_await adl_coawait::Awaitable{93};
  co_return value + 4;
}

Task coroutine_member_operator_wins_over_adl(void) {
  adl_coawait::MemberPreferredAwaitable awaitable(94);
  int value = co_await awaitable;
  co_return value + 4;
}

Task coroutine_direct_await_keeps_local(int input) {
  int kept = input + 1;
  int value = co_await DirectSuspendValue{19};
  co_return kept + value;
}

Task coroutine_direct_await_loop_expression(void) {
  int sum = 0;
  int i = 0;
  while (i < 2) {
    int value = co_await DirectSuspendValue{30 + i};
    sum = sum + value;
    i = i + 1;
  }
  co_return sum;
}

Task coroutine_direct_await_factory_loop_expression(void) {
  int sum = 0;
  int i = 0;
  while (i < 2) {
    int value = co_await make_suspend_value(40 + i);
    sum = sum + value;
    i = i + 1;
  }
  co_return sum;
}

Task coroutine_yield_twice(void) {
  yield_twice_start_count = yield_twice_start_count + 1;
  co_yield 101;
  co_yield 202;
  co_return 303;
}

struct InitialSuspendAlways {
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    initial_always_suspend_count = initial_always_suspend_count + 1;
    last_frame_handle = handle;
  }
  void await_resume(void) {
    initial_always_resume_count = initial_always_resume_count + 1;
  }
};

struct InitialTask;
typedef struct InitialTask InitialTask;
struct InitialPromise;

struct InitialTask {
  using promise_type = InitialPromise;
  int value;
  void* handle;
};

struct InitialPromise {
  int value;
  InitialTask get_return_object(void) {
    InitialTask task = {value, last_frame_handle};
    return task;
  }

  InitialSuspendAlways initial_suspend(void) {
    InitialSuspendAlways value = {};
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

  void unhandled_exception(void) {
  }
};

InitialTask coroutine_initially_suspended(void) {
  initial_suspend_body_count = initial_suspend_body_count + 1;
  co_return 404;
}

struct LifetimeTask;
typedef struct LifetimeTask LifetimeTask;
struct LifetimePromise;

struct LifetimeTask {
  using promise_type = LifetimePromise;
  int value;
  void* handle;
};

struct LifetimePromise {
  int value;
  LifetimePromise();
  ~LifetimePromise();

  LifetimeTask get_return_object(void) {
    LifetimeTask task = {value, last_frame_handle};
    return task;
  }

  SuspendNever initial_suspend(void) {
    SuspendNever value = {};
    return value;
  }

  SuspendNever final_suspend(void) {
    SuspendNever value = {};
    return value;
  }

  void return_value(int result) {
    value = result;
  }

  void unhandled_exception(void) {
  }
};

LifetimePromise::LifetimePromise() {
  lifetime_promise_ctor_count = lifetime_promise_ctor_count + 1;
  value = 0;
}

LifetimePromise::~LifetimePromise() {
  lifetime_promise_dtor_count = lifetime_promise_dtor_count + 1;
}

LifetimeTask coroutine_lifetime_suspend(void) {
  SuspendValue awaiter = {77};
  int value = co_await awaiter;
  co_return value + 1;
}

Task coroutine_tracked_awaiter_suspend(void) {
  TrackedAwaiter awaiter(91);
  int value = co_await awaiter;
  co_return value + 2;
}

Task coroutine_persist_local_suspend(void) {
  int kept = 31;
  SuspendValue awaiter = {4};
  int value = co_await awaiter;
  co_return kept + value;
}

Task coroutine_persist_local_mutation_suspend(void) {
  int kept = 10;
  kept = kept + 5;
  SuspendValue awaiter = {4};
  int value = co_await awaiter;
  kept = kept + value;
  co_return kept + 1;
}

Task coroutine_persist_class_local_suspend(void) {
  PersistedLocalBox kept(40);
  SuspendValue awaiter = {6};
  int value = co_await awaiter;
  co_return kept.value + value;
}

Task coroutine_persist_parameter_suspend(int input) {
  SuspendValue awaiter = {6};
  int value = co_await awaiter;
  co_return input + value;
}

Task coroutine_persist_movable_parameter_suspend(PersistedParamBox input) {
  SuspendValue awaiter = {6};
  int value = co_await awaiter;
  co_return input.value + value;
}

Task coroutine_nested_block_suspend(int input) {
  int kept = input + 3;
  {
    SuspendValue awaiter = {7};
    int value = co_await awaiter;
    kept = kept + value;
  }
  co_return kept + 1;
}

Task coroutine_nested_if_suspend(int flag, int input) {
  int kept = input;
  if (flag) {
    SuspendValue awaiter = {8};
    int value = co_await awaiter;
    kept = kept + value;
  }
  co_return kept + 2;
}

Task coroutine_nested_else_suspend(int flag, int input) {
  int kept = input;
  if (flag) {
    kept = kept + 5;
  } else {
    SuspendValue awaiter = {9};
    int value = co_await awaiter;
    kept = kept + value;
  }
  co_return kept + 3;
}

Task coroutine_nested_if_else_suspend(int flag, int input) {
  int kept = input;
  if (flag) {
    SuspendValue awaiter = {10};
    int value = co_await awaiter;
    kept = kept + value;
  } else {
    SuspendValue awaiter = {11};
    int value = co_await awaiter;
    kept = kept + value + 20;
  }
  co_return kept + 4;
}

Task coroutine_nested_branch_local_suspend(void) {
  int result = 0;
  if (1) {
    PersistedLocalBox kept(5);
    SuspendValue awaiter = {6};
    int value = co_await awaiter;
    result = kept.value + value;
  }
  co_return result;
}

Task coroutine_while_loop_suspend(void) {
  int sum = 0;
  int i = 0;
  while (i < 2) {
    int kept = i + 3;
    SuspendValue awaiter = {10 + i};
    int value = co_await awaiter;
    sum = sum + kept + value;
    i = i + 1;
  }
  co_return sum + 1;
}

Task coroutine_for_loop_suspend(void) {
  int sum = 0;
  int i = 0;
  for (; i < 2; i = i + 1) {
    int kept = i + 5;
    SuspendValue awaiter = {20 + i};
    int value = co_await awaiter;
    sum = sum + kept + value;
  }
  co_return sum + 2;
}

Task coroutine_nested_if_yield(int flag) {
  if (flag) {
    co_yield 401;
  } else {
    co_yield 402;
  }
  co_return 403;
}

Task coroutine_while_loop_yield(void) {
  int i = 0;
  while (i < 2) {
    co_yield 501 + i;
    i = i + 1;
  }
  co_return 503;
}

Task coroutine_for_loop_yield(void) {
  int i = 0;
  for (; i < 2; i = i + 1) {
    co_yield 601 + i;
  }
  co_return 603;
}

Task coroutine_branch_multiple_suspend(int flag) {
  if (flag) {
    SuspendValue awaiter = {7};
    int value = co_await awaiter;
    co_yield value + 700;
  } else {
    co_yield 709;
  }
  co_return 710;
}

Task coroutine_mixed_await_yield_sequence(int input) {
  int sum = input;
  {
    SuspendValue first = {11};
    int first_value = co_await first;
    sum = sum + first_value;
    co_yield sum + 20;
    SuspendValue second = {13};
    int second_value = co_await second;
    co_return sum + second_value;
  }
}

Task coroutine_do_while_yield(void) {
  int i = 0;
  do {
    co_yield 801 + i;
    i = i + 1;
  } while (i < 2);
  co_return 803;
}

Task coroutine_nested_loop_suspend(void) {
  int sum = 0;
  int base = 20;
  int i = 0;
  while (i < 2) {
    int j = 0;
    while (j < 2) {
      SuspendValue awaiter = {base + j};
      int value = co_await awaiter;
      sum = sum + value;
      j = j + 1;
    }
    base = base + 10;
    i = i + 1;
  }
  co_return sum;
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
  if (handle == 0 || frame_done(handle)) {
    return;
  }
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  if (frame->resume == 0) {
    return;
  }
  Task task = frame->resume(handle);
  last_resume_value = task.value;
}

void resume_yield_coroutine(void* handle) {
  last_resume_value = -1;
  if (handle == 0 || frame_done(handle)) {
    return;
  }
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  if (frame->resume == 0) {
    return;
  }
  Task task = frame->resume(handle);
  last_resume_value = task.value;
}

void destroy_coroutine(void* handle) {
  if (handle == 0) {
    return;
  }
  CoroutineFrame* frame = (CoroutineFrame*)handle;
  if (frame->destroy != 0) {
    frame->destroy(handle);
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

  int bool_await_suspend_before = bool_await_suspend_count;
  Task bool_continue_started = coroutine_bool_await_suspend_continue();
  if (bool_await_suspend_count != bool_await_suspend_before + 1 ||
      bool_continue_started.value != 105 ||
      bool_continue_started.handle == 0 ||
      frame_state(bool_continue_started.handle) != 0 ||
      !frame_done(bool_continue_started.handle)) {
    return 158;
  }
  destroy_coroutine(bool_continue_started.handle);

  Task bool_suspend_started = coroutine_bool_await_suspend_suspend();
  if (bool_await_suspend_count != bool_await_suspend_before + 2 ||
      bool_suspend_started.value != 0 ||
      bool_suspend_started.handle == 0 ||
      frame_state(bool_suspend_started.handle) != 1 ||
      frame_done(bool_suspend_started.handle)) {
    return 159;
  }
  resume_coroutine(bool_suspend_started.handle);
  if (last_resume_value != 107 ||
      frame_state(bool_suspend_started.handle) != 0 ||
      !frame_done(bool_suspend_started.handle)) {
    return 160;
  }
  destroy_coroutine(bool_suspend_started.handle);

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
  if (final_suspend_count != 7) {
    return 12;
  }
  CoroutineFrame* completed_twice_frame = (CoroutineFrame*)last_frame_handle;
  if (completed_twice_frame->resume != 0 ||
      completed_twice_frame->destroy == 0) {
    return 28;
  }

  resume_coroutine(last_frame_handle);
  if (last_resume_value != -1 || final_suspend_count != 7) {
    return 13;
  }
  destroy_coroutine(last_frame_handle);
  if (frame_state(last_frame_handle) != 0 || !frame_done(last_frame_handle) ||
      completed_twice_frame->resume != 0 ||
      completed_twice_frame->destroy != 0 || final_suspend_count != 7) {
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
      final_suspend_count != 7) {
    return 26;
  }
  destroy_coroutine(destroy_started.handle);
  if (last_resume_value != -1 || suspend_twice_start_count != 2 ||
      final_suspend_count != 7) {
    return 27;
  }

  Task live_a = coroutine_suspend_twice_unique();
  void* handle_a = live_a.handle;
  Task live_b = coroutine_suspend_twice_unique();
  void* handle_b = live_b.handle;
  if (handle_a == 0 || handle_b == 0 || handle_a == handle_b ||
      frame_state(handle_a) != 1 || frame_state(handle_b) != 1 ||
      frame_done(handle_a) || frame_done(handle_b)) {
    return 37;
  }
  if (suspend_twice_start_count != 2 || suspend_unique_start_count != 2 ||
      suspend_unique_next_base != 200) {
    return 38;
  }

  resume_coroutine(handle_a);
  if (last_resume_value != 0 || frame_state(handle_a) != 2 ||
      frame_state(handle_b) != 1 || frame_done(handle_a) ||
      frame_done(handle_b)) {
    return 39;
  }

  resume_coroutine(handle_b);
  if (last_resume_value != 0 || frame_state(handle_a) != 2 ||
      frame_state(handle_b) != 2 || frame_done(handle_a) ||
      frame_done(handle_b)) {
    return 40;
  }

  resume_coroutine(handle_a);
  if (last_resume_value != 38 || frame_state(handle_a) != 0 ||
      !frame_done(handle_a) || frame_state(handle_b) != 2 ||
      frame_done(handle_b)) {
    return 41;
  }

  resume_coroutine(handle_b);
  if (last_resume_value != 138 || frame_state(handle_b) != 0 ||
      !frame_done(handle_b) || final_suspend_count != 9) {
    return 42;
  }
  destroy_coroutine(handle_a);
  destroy_coroutine(handle_b);
  if (promise_coroutine_operator_new_count != 7 ||
      promise_coroutine_operator_delete_count != 6 ||
      global_coroutine_operator_new_count != 0 ||
      global_coroutine_operator_delete_count != 0) {
    return 43;
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
  if (final_suspend_count != 10) {
    return 17;
  }

  resume_yield_coroutine(last_frame_handle);
  if (last_resume_value != -1 || final_suspend_count != 10) {
    return 18;
  }
  if (initial_suspend_count != 11) {
    return 31;
  }

  last_frame_handle = 0;
  InitialTask initially_suspended = coroutine_initially_suspended();
  if (initially_suspended.value != 0 || initially_suspended.handle == 0 ||
      initially_suspended.handle != last_frame_handle ||
      frame_state(initially_suspended.handle) != 0 ||
      frame_done(initially_suspended.handle)) {
    return 32;
  }
  if (initial_always_suspend_count != 1 ||
      initial_always_resume_count != 0 ||
      initial_suspend_body_count != 0) {
    return 33;
  }

  resume_coroutine(initially_suspended.handle);
  if (last_resume_value != 404 || initial_always_suspend_count != 1 ||
      initial_always_resume_count != 1 ||
      initial_suspend_body_count != 1) {
    return 34;
  }
  CoroutineFrame* initially_suspended_frame =
      (CoroutineFrame*)initially_suspended.handle;
  if (frame_state(initially_suspended.handle) != 0 ||
      !frame_done(initially_suspended.handle) ||
      initially_suspended_frame->resume != 0 ||
      initially_suspended_frame->destroy == 0 ||
      final_suspend_count != 11) {
    return 35;
  }

  resume_coroutine(initially_suspended.handle);
  if (last_resume_value != -1 || initial_suspend_body_count != 1 ||
      initial_always_resume_count != 1 || final_suspend_count != 11) {
    return 36;
  }

  LifetimeTask lifetime_started = coroutine_lifetime_suspend();
  if (lifetime_promise_ctor_count != 1 ||
      lifetime_promise_dtor_count != 0 ||
      lifetime_started.value != 0 || lifetime_started.handle == 0 ||
      frame_state(lifetime_started.handle) != 1) {
    return 45;
  }
  resume_coroutine(lifetime_started.handle);
  if (last_resume_value != 78 ||
      lifetime_promise_ctor_count != 1 ||
      lifetime_promise_dtor_count != 0 ||
      frame_state(lifetime_started.handle) != 0 ||
      !frame_done(lifetime_started.handle)) {
    return 46;
  }
  destroy_coroutine(lifetime_started.handle);
  if (lifetime_promise_ctor_count != 1 ||
      lifetime_promise_dtor_count != 1) {
    return 47;
  }

  Task tracked_started = coroutine_tracked_awaiter_suspend();
  if (tracked_awaiter_ctor_count != 1 ||
      tracked_awaiter_copy_count != 1 ||
      tracked_started.value != 0 || tracked_started.handle == 0 ||
      frame_state(tracked_started.handle) != 1) {
    return 48;
  }
  int tracked_dtor_before_destroy = tracked_awaiter_dtor_count;
  resume_coroutine(tracked_started.handle);
  if (last_resume_value != 93 ||
      tracked_awaiter_ctor_count != 1 ||
      tracked_awaiter_copy_count != 1 ||
      frame_state(tracked_started.handle) != 0 ||
      !frame_done(tracked_started.handle)) {
    return 49;
  }
  destroy_coroutine(tracked_started.handle);
  if (tracked_awaiter_dtor_count != tracked_dtor_before_destroy + 1) {
    return 50;
  }

  Task persist_started = coroutine_persist_local_suspend();
  if (persist_started.value != 0 || persist_started.handle == 0 ||
      frame_state(persist_started.handle) != 1) {
    return 51;
  }
  resume_coroutine(persist_started.handle);
  if (last_resume_value != 35 || frame_state(persist_started.handle) != 0 ||
      !frame_done(persist_started.handle)) {
    return 52;
  }
  destroy_coroutine(persist_started.handle);

  Task persist_mutation_started = coroutine_persist_local_mutation_suspend();
  if (persist_mutation_started.value != 0 ||
      persist_mutation_started.handle == 0 ||
      frame_state(persist_mutation_started.handle) != 1) {
    return 53;
  }
  resume_coroutine(persist_mutation_started.handle);
  if (last_resume_value != 20 ||
      frame_state(persist_mutation_started.handle) != 0 ||
      !frame_done(persist_mutation_started.handle)) {
    return 54;
  }
  destroy_coroutine(persist_mutation_started.handle);

  Task persist_class_local_started = coroutine_persist_class_local_suspend();
  if (persist_class_local_started.value != 0 ||
      persist_class_local_started.handle == 0 ||
      persisted_local_ctor_count != 1 ||
      persisted_local_copy_count != 1 ||
      frame_state(persist_class_local_started.handle) != 1) {
    return 60;
  }
  resume_coroutine(persist_class_local_started.handle);
  if (last_resume_value != 246 ||
      frame_state(persist_class_local_started.handle) != 0 ||
      !frame_done(persist_class_local_started.handle)) {
    return 61;
  }
  int persisted_local_dtor_before_destroy = persisted_local_dtor_count;
  destroy_coroutine(persist_class_local_started.handle);
  if (persisted_local_dtor_count != persisted_local_dtor_before_destroy + 1) {
    return 62;
  }

  Task persist_parameter_started = coroutine_persist_parameter_suspend(70);
  if (persist_parameter_started.value != 0 ||
      persist_parameter_started.handle == 0 ||
      frame_state(persist_parameter_started.handle) != 1) {
    return 55;
  }
  resume_coroutine(persist_parameter_started.handle);
  if (last_resume_value != 76 ||
      frame_state(persist_parameter_started.handle) != 0 ||
      !frame_done(persist_parameter_started.handle)) {
    return 56;
  }
  destroy_coroutine(persist_parameter_started.handle);

  PersistedParamBox persisted_box(80);
  Task persist_movable_parameter_started =
      coroutine_persist_movable_parameter_suspend(persisted_box);
  if (persist_movable_parameter_started.value != 0 ||
      persist_movable_parameter_started.handle == 0 ||
      persisted_param_ctor_count != 1 ||
      persisted_param_move_count != 1 ||
      frame_state(persist_movable_parameter_started.handle) != 1) {
    return 57;
  }
  resume_coroutine(persist_movable_parameter_started.handle);
  if (last_resume_value != 186 ||
      frame_state(persist_movable_parameter_started.handle) != 0 ||
      !frame_done(persist_movable_parameter_started.handle)) {
    return 58;
  }
  int persisted_param_dtor_before_destroy = persisted_param_dtor_count;
  destroy_coroutine(persist_movable_parameter_started.handle);
  if (persisted_param_dtor_count != persisted_param_dtor_before_destroy + 1) {
    return 59;
  }

  Task nested_block_started = coroutine_nested_block_suspend(20);
  if (nested_block_started.value != 0 || nested_block_started.handle == 0 ||
      frame_state(nested_block_started.handle) != 1) {
    return 63;
  }
  resume_coroutine(nested_block_started.handle);
  if (last_resume_value != 31 ||
      frame_state(nested_block_started.handle) != 0 ||
      !frame_done(nested_block_started.handle)) {
    return 64;
  }
  destroy_coroutine(nested_block_started.handle);

  Task nested_if_started = coroutine_nested_if_suspend(1, 30);
  if (nested_if_started.value != 0 || nested_if_started.handle == 0 ||
      frame_state(nested_if_started.handle) != 1) {
    return 65;
  }
  resume_coroutine(nested_if_started.handle);
  if (last_resume_value != 40 ||
      frame_state(nested_if_started.handle) != 0 ||
      !frame_done(nested_if_started.handle)) {
    return 66;
  }
  destroy_coroutine(nested_if_started.handle);

  Task nested_else_started = coroutine_nested_else_suspend(0, 30);
  if (nested_else_started.value != 0 || nested_else_started.handle == 0 ||
      frame_state(nested_else_started.handle) != 1) {
    return 67;
  }
  resume_coroutine(nested_else_started.handle);
  if (last_resume_value != 42 ||
      frame_state(nested_else_started.handle) != 0 ||
      !frame_done(nested_else_started.handle)) {
    return 68;
  }
  destroy_coroutine(nested_else_started.handle);

  Task nested_if_else_true_started = coroutine_nested_if_else_suspend(1, 30);
  if (nested_if_else_true_started.value != 0 ||
      nested_if_else_true_started.handle == 0 ||
      frame_state(nested_if_else_true_started.handle) != 1) {
    return 69;
  }
  resume_coroutine(nested_if_else_true_started.handle);
  if (last_resume_value != 44 ||
      frame_state(nested_if_else_true_started.handle) != 0 ||
      !frame_done(nested_if_else_true_started.handle)) {
    return 70;
  }
  destroy_coroutine(nested_if_else_true_started.handle);

  Task nested_if_else_false_started = coroutine_nested_if_else_suspend(0, 30);
  if (nested_if_else_false_started.value != 0 ||
      nested_if_else_false_started.handle == 0 ||
      frame_state(nested_if_else_false_started.handle) != 2) {
    return 71;
  }
  resume_coroutine(nested_if_else_false_started.handle);
  if (last_resume_value != 65 ||
      frame_state(nested_if_else_false_started.handle) != 0 ||
      !frame_done(nested_if_else_false_started.handle)) {
    return 72;
  }
  destroy_coroutine(nested_if_else_false_started.handle);

  Task nested_branch_local_started = coroutine_nested_branch_local_suspend();
  if (nested_branch_local_started.value != 0 ||
      nested_branch_local_started.handle == 0 ||
      frame_state(nested_branch_local_started.handle) != 1 ||
      persisted_local_copy_count != 2) {
    return 73;
  }
  resume_coroutine(nested_branch_local_started.handle);
  if (last_resume_value != 211 ||
      frame_state(nested_branch_local_started.handle) != 0 ||
      !frame_done(nested_branch_local_started.handle)) {
    return 74;
  }
  destroy_coroutine(nested_branch_local_started.handle);

  Task while_loop_started = coroutine_while_loop_suspend();
  if (while_loop_started.value != 0 || while_loop_started.handle == 0 ||
      frame_state(while_loop_started.handle) != 1) {
    return 75;
  }
  resume_coroutine(while_loop_started.handle);
  if (last_resume_value != 0 ||
      frame_state(while_loop_started.handle) != 1 ||
      frame_done(while_loop_started.handle)) {
    return 76;
  }
  resume_coroutine(while_loop_started.handle);
  if (last_resume_value != 29 ||
      frame_state(while_loop_started.handle) != 0 ||
      !frame_done(while_loop_started.handle)) {
    return 77;
  }
  destroy_coroutine(while_loop_started.handle);

  Task for_loop_started = coroutine_for_loop_suspend();
  if (for_loop_started.value != 0 || for_loop_started.handle == 0 ||
      frame_state(for_loop_started.handle) != 1) {
    return 78;
  }
  resume_coroutine(for_loop_started.handle);
  if (last_resume_value != 0 ||
      frame_state(for_loop_started.handle) != 1 ||
      frame_done(for_loop_started.handle)) {
    return 79;
  }
  resume_coroutine(for_loop_started.handle);
  if (last_resume_value != 54 ||
      frame_state(for_loop_started.handle) != 0 ||
      !frame_done(for_loop_started.handle)) {
    return 80;
  }
  destroy_coroutine(for_loop_started.handle);

  Task nested_if_yield_true = coroutine_nested_if_yield(1);
  if (nested_if_yield_true.value != 401 ||
      nested_if_yield_true.handle == 0 ||
      frame_state(nested_if_yield_true.handle) != 1) {
    return 81;
  }
  resume_yield_coroutine(nested_if_yield_true.handle);
  if (last_resume_value != 403 ||
      frame_state(nested_if_yield_true.handle) != 0 ||
      !frame_done(nested_if_yield_true.handle)) {
    return 82;
  }
  destroy_coroutine(nested_if_yield_true.handle);

  Task nested_if_yield_false = coroutine_nested_if_yield(0);
  if (nested_if_yield_false.value != 402 ||
      nested_if_yield_false.handle == 0 ||
      frame_state(nested_if_yield_false.handle) != 2) {
    return 83;
  }
  resume_yield_coroutine(nested_if_yield_false.handle);
  if (last_resume_value != 403 ||
      frame_state(nested_if_yield_false.handle) != 0 ||
      !frame_done(nested_if_yield_false.handle)) {
    return 84;
  }
  destroy_coroutine(nested_if_yield_false.handle);

  Task while_yield_started = coroutine_while_loop_yield();
  if (while_yield_started.value != 501 || while_yield_started.handle == 0 ||
      frame_state(while_yield_started.handle) != 1) {
    return 85;
  }
  resume_yield_coroutine(while_yield_started.handle);
  if (last_resume_value != 502 ||
      frame_state(while_yield_started.handle) != 1 ||
      frame_done(while_yield_started.handle)) {
    return 86;
  }
  resume_yield_coroutine(while_yield_started.handle);
  if (last_resume_value != 503 ||
      frame_state(while_yield_started.handle) != 0 ||
      !frame_done(while_yield_started.handle)) {
    return 87;
  }
  destroy_coroutine(while_yield_started.handle);

  Task for_yield_started = coroutine_for_loop_yield();
  if (for_yield_started.value != 601 || for_yield_started.handle == 0 ||
      frame_state(for_yield_started.handle) != 1) {
    return 88;
  }
  resume_yield_coroutine(for_yield_started.handle);
  if (last_resume_value != 602 ||
      frame_state(for_yield_started.handle) != 1 ||
      frame_done(for_yield_started.handle)) {
    return 89;
  }
  resume_yield_coroutine(for_yield_started.handle);
  if (last_resume_value != 603 ||
      frame_state(for_yield_started.handle) != 0 ||
      !frame_done(for_yield_started.handle)) {
    return 90;
  }
  destroy_coroutine(for_yield_started.handle);

  Task branch_multiple_started = coroutine_branch_multiple_suspend(1);
  if (branch_multiple_started.value != 0 ||
      branch_multiple_started.handle == 0 ||
      frame_state(branch_multiple_started.handle) != 1) {
    return 91;
  }
  resume_yield_coroutine(branch_multiple_started.handle);
  if (last_resume_value != 707 ||
      frame_state(branch_multiple_started.handle) != 2 ||
      frame_done(branch_multiple_started.handle)) {
    return 92;
  }
  resume_yield_coroutine(branch_multiple_started.handle);
  if (last_resume_value != 710 ||
      frame_state(branch_multiple_started.handle) != 0 ||
      !frame_done(branch_multiple_started.handle)) {
    return 93;
  }
  destroy_coroutine(branch_multiple_started.handle);

  Task branch_multiple_else_started = coroutine_branch_multiple_suspend(0);
  if (branch_multiple_else_started.value != 709 ||
      branch_multiple_else_started.handle == 0 ||
      frame_state(branch_multiple_else_started.handle) != 3) {
    return 94;
  }
  resume_yield_coroutine(branch_multiple_else_started.handle);
  if (last_resume_value != 710 ||
      frame_state(branch_multiple_else_started.handle) != 0 ||
      !frame_done(branch_multiple_else_started.handle)) {
    return 95;
  }
  destroy_coroutine(branch_multiple_else_started.handle);

  Task mixed_started = coroutine_mixed_await_yield_sequence(5);
  if (mixed_started.value != 0 || mixed_started.handle == 0 ||
      frame_state(mixed_started.handle) != 1) {
    return 96;
  }
  resume_yield_coroutine(mixed_started.handle);
  if (last_resume_value != 36 ||
      frame_state(mixed_started.handle) != 2 ||
      frame_done(mixed_started.handle)) {
    return 97;
  }
  resume_yield_coroutine(mixed_started.handle);
  if (last_resume_value != 36 || frame_state(mixed_started.handle) != 3 ||
      frame_done(mixed_started.handle)) {
    return 98;
  }
  resume_yield_coroutine(mixed_started.handle);
  if (last_resume_value != 29 ||
      frame_state(mixed_started.handle) != 0 ||
      !frame_done(mixed_started.handle)) {
    return 99;
  }
  destroy_coroutine(mixed_started.handle);

  Task do_while_yield_started = coroutine_do_while_yield();
  if (do_while_yield_started.value != 801 ||
      do_while_yield_started.handle == 0 ||
      frame_state(do_while_yield_started.handle) != 1) {
    return 100;
  }
  resume_yield_coroutine(do_while_yield_started.handle);
  if (last_resume_value != 802 ||
      frame_state(do_while_yield_started.handle) != 1 ||
      frame_done(do_while_yield_started.handle)) {
    return 101;
  }
  resume_yield_coroutine(do_while_yield_started.handle);
  if (last_resume_value != 803 ||
      frame_state(do_while_yield_started.handle) != 0 ||
      !frame_done(do_while_yield_started.handle)) {
    return 102;
  }
  destroy_coroutine(do_while_yield_started.handle);

  Task nested_loop_started = coroutine_nested_loop_suspend();
  if (nested_loop_started.value != 0 || nested_loop_started.handle == 0 ||
      frame_state(nested_loop_started.handle) != 1) {
    return 103;
  }
  resume_coroutine(nested_loop_started.handle);
  if (last_resume_value != 0 || frame_state(nested_loop_started.handle) != 1 ||
      frame_done(nested_loop_started.handle)) {
    return 104;
  }
  resume_coroutine(nested_loop_started.handle);
  if (last_resume_value != 0 || frame_state(nested_loop_started.handle) != 1 ||
      frame_done(nested_loop_started.handle)) {
    return 105;
  }
  resume_coroutine(nested_loop_started.handle);
  if (last_resume_value != 0 || frame_state(nested_loop_started.handle) != 1 ||
      frame_done(nested_loop_started.handle)) {
    return 106;
  }
  resume_coroutine(nested_loop_started.handle);
  if (last_resume_value != 102 ||
      frame_state(nested_loop_started.handle) != 0 ||
      !frame_done(nested_loop_started.handle)) {
    return 107;
  }
  destroy_coroutine(nested_loop_started.handle);

  Task direct_await_started = coroutine_direct_await_expression();
  if (direct_await_started.value != 0 || direct_await_started.handle == 0 ||
      frame_state(direct_await_started.handle) != 1) {
    return 108;
  }
  resume_coroutine(direct_await_started.handle);
  if (last_resume_value != 21) {
    return 109;
  }
  if (frame_state(direct_await_started.handle) != 0) {
    return 115;
  }
  if (!frame_done(direct_await_started.handle)) {
    return 116;
  }
  destroy_coroutine(direct_await_started.handle);

  Task direct_await_aggregate_started =
      coroutine_direct_await_aggregate_expression();
  if (direct_await_aggregate_started.value != 0 ||
      direct_await_aggregate_started.handle == 0 ||
      frame_state(direct_await_aggregate_started.handle) != 1) {
    return 117;
  }
  resume_coroutine(direct_await_aggregate_started.handle);
  if (last_resume_value != 27 ||
      frame_state(direct_await_aggregate_started.handle) != 0 ||
      !frame_done(direct_await_aggregate_started.handle)) {
    return 118;
  }
  destroy_coroutine(direct_await_aggregate_started.handle);

  Task direct_await_factory_started =
      coroutine_direct_await_factory_expression();
  if (direct_await_factory_started.value != 0 ||
      direct_await_factory_started.handle == 0 ||
      frame_state(direct_await_factory_started.handle) != 1) {
    return 119;
  }
  resume_coroutine(direct_await_factory_started.handle);
  if (last_resume_value != 29 ||
      frame_state(direct_await_factory_started.handle) != 0 ||
      !frame_done(direct_await_factory_started.handle)) {
    return 120;
  }
  destroy_coroutine(direct_await_factory_started.handle);

  Task member_operator_named_started =
      coroutine_member_operator_co_await_named();
  if (member_operator_named_started.value != 0 ||
      member_operator_named_started.handle == 0 ||
      frame_state(member_operator_named_started.handle) != 1) {
    return 124;
  }
  resume_coroutine(member_operator_named_started.handle);
  if (last_resume_value != 41 ||
      frame_state(member_operator_named_started.handle) != 0 ||
      !frame_done(member_operator_named_started.handle)) {
    return 125;
  }
  destroy_coroutine(member_operator_named_started.handle);

  Task member_operator_temporary_started =
      coroutine_member_operator_co_await_temporary();
  if (member_operator_temporary_started.value != 0 ||
      member_operator_temporary_started.handle == 0 ||
      frame_state(member_operator_temporary_started.handle) != 1) {
    return 126;
  }
  if (member_coawait_temp_dtor_count != 1) {
    return 134;
  }
  resume_coroutine(member_operator_temporary_started.handle);
  if (last_resume_value != 51) {
    return 127;
  }
  if (frame_state(member_operator_temporary_started.handle) != 0) {
    return 128;
  }
  if (!frame_done(member_operator_temporary_started.handle)) {
    return 129;
  }
  destroy_coroutine(member_operator_temporary_started.handle);

  Task member_operator_destroy_started =
      coroutine_member_operator_co_await_temporary();
  if (member_operator_destroy_started.value != 0 ||
      member_operator_destroy_started.handle == 0 ||
      frame_state(member_operator_destroy_started.handle) != 1) {
    return 135;
  }
  if (member_coawait_temp_dtor_count != 2) {
    return 136;
  }
  destroy_coroutine(member_operator_destroy_started.handle);
  if (member_coawait_temp_dtor_count != 2) {
    return 137;
  }

  Task free_operator_named_started =
      coroutine_free_operator_co_await_named();
  if (free_operator_named_started.value != 0 ||
      free_operator_named_started.handle == 0 ||
      frame_state(free_operator_named_started.handle) != 1) {
    return 130;
  }
  resume_coroutine(free_operator_named_started.handle);
  if (last_resume_value != 62 ||
      frame_state(free_operator_named_started.handle) != 0 ||
      !frame_done(free_operator_named_started.handle)) {
    return 131;
  }
  destroy_coroutine(free_operator_named_started.handle);

  int free_coawait_temp_dtor_before = free_coawait_temp_dtor_count;
  Task free_operator_temporary_started =
      coroutine_free_operator_co_await_temporary();
  if (free_operator_temporary_started.value != 0 ||
      free_operator_temporary_started.handle == 0 ||
      frame_state(free_operator_temporary_started.handle) != 1) {
    return 132;
  }
  if (free_coawait_temp_dtor_count != free_coawait_temp_dtor_before + 1) {
    return 138;
  }
  resume_coroutine(free_operator_temporary_started.handle);
  if (last_resume_value != 72 ||
      frame_state(free_operator_temporary_started.handle) != 0 ||
      !frame_done(free_operator_temporary_started.handle)) {
    return 133;
  }
  destroy_coroutine(free_operator_temporary_started.handle);

  Task free_operator_destroy_started =
      coroutine_free_operator_co_await_temporary();
  if (free_operator_destroy_started.value != 0 ||
      free_operator_destroy_started.handle == 0 ||
      frame_state(free_operator_destroy_started.handle) != 1) {
    return 139;
  }
  if (free_coawait_temp_dtor_count != free_coawait_temp_dtor_before + 2) {
    return 140;
  }
  destroy_coroutine(free_operator_destroy_started.handle);
  if (free_coawait_temp_dtor_count != free_coawait_temp_dtor_before + 2) {
    return 141;
  }

  Task free_operator_second_started =
      coroutine_free_operator_co_await_second_overload();
  if (free_operator_second_started.value != 0 ||
      free_operator_second_started.handle == 0 ||
      frame_state(free_operator_second_started.handle) != 1) {
    return 142;
  }
  resume_coroutine(free_operator_second_started.handle);
  if (last_resume_value != 83 ||
      frame_state(free_operator_second_started.handle) != 0 ||
      !frame_done(free_operator_second_started.handle)) {
    return 143;
  }
  destroy_coroutine(free_operator_second_started.handle);

  Task free_operator_cvref_lvalue_started =
      coroutine_free_operator_co_await_cvref_lvalue();
  if (free_operator_cvref_lvalue_started.value != 0 ||
      free_operator_cvref_lvalue_started.handle == 0 ||
      frame_state(free_operator_cvref_lvalue_started.handle) != 1) {
    return 144;
  }
  resume_coroutine(free_operator_cvref_lvalue_started.handle);
  if (last_resume_value != 95 ||
      frame_state(free_operator_cvref_lvalue_started.handle) != 0 ||
      !frame_done(free_operator_cvref_lvalue_started.handle)) {
    return 145;
  }
  destroy_coroutine(free_operator_cvref_lvalue_started.handle);

  Task free_operator_cvref_const_lvalue_started =
      coroutine_free_operator_co_await_cvref_const_lvalue();
  if (free_operator_cvref_const_lvalue_started.value != 0 ||
      free_operator_cvref_const_lvalue_started.handle == 0 ||
      frame_state(free_operator_cvref_const_lvalue_started.handle) != 1) {
    return 146;
  }
  resume_coroutine(free_operator_cvref_const_lvalue_started.handle);
  if (last_resume_value != 106 ||
      frame_state(free_operator_cvref_const_lvalue_started.handle) != 0 ||
      !frame_done(free_operator_cvref_const_lvalue_started.handle)) {
    return 147;
  }
  destroy_coroutine(free_operator_cvref_const_lvalue_started.handle);

  Task free_operator_cvref_rvalue_started =
      coroutine_free_operator_co_await_cvref_rvalue();
  if (free_operator_cvref_rvalue_started.value != 0 ||
      free_operator_cvref_rvalue_started.handle == 0 ||
      frame_state(free_operator_cvref_rvalue_started.handle) != 1) {
    return 148;
  }
  resume_coroutine(free_operator_cvref_rvalue_started.handle);
  if (last_resume_value != 117 ||
      frame_state(free_operator_cvref_rvalue_started.handle) != 0 ||
      !frame_done(free_operator_cvref_rvalue_started.handle)) {
    return 149;
  }
  destroy_coroutine(free_operator_cvref_rvalue_started.handle);

  Task adl_operator_lvalue_started = coroutine_adl_operator_co_await_lvalue();
  if (adl_operator_lvalue_started.value != 0 ||
      adl_operator_lvalue_started.handle == 0 ||
      frame_state(adl_operator_lvalue_started.handle) != 1) {
    return 150;
  }
  resume_coroutine(adl_operator_lvalue_started.handle);
  if (last_resume_value != 135 ||
      frame_state(adl_operator_lvalue_started.handle) != 0 ||
      !frame_done(adl_operator_lvalue_started.handle)) {
    return 151;
  }
  destroy_coroutine(adl_operator_lvalue_started.handle);

  Task adl_operator_const_lvalue_started =
      coroutine_adl_operator_co_await_const_lvalue();
  if (adl_operator_const_lvalue_started.value != 0 ||
      adl_operator_const_lvalue_started.handle == 0 ||
      frame_state(adl_operator_const_lvalue_started.handle) != 1) {
    return 152;
  }
  resume_coroutine(adl_operator_const_lvalue_started.handle);
  if (last_resume_value != 146 ||
      frame_state(adl_operator_const_lvalue_started.handle) != 0 ||
      !frame_done(adl_operator_const_lvalue_started.handle)) {
    return 153;
  }
  destroy_coroutine(adl_operator_const_lvalue_started.handle);

  Task adl_operator_rvalue_started = coroutine_adl_operator_co_await_rvalue();
  if (adl_operator_rvalue_started.value != 0 ||
      adl_operator_rvalue_started.handle == 0 ||
      frame_state(adl_operator_rvalue_started.handle) != 1) {
    return 154;
  }
  resume_coroutine(adl_operator_rvalue_started.handle);
  if (last_resume_value != 157 ||
      frame_state(adl_operator_rvalue_started.handle) != 0 ||
      !frame_done(adl_operator_rvalue_started.handle)) {
    return 155;
  }
  destroy_coroutine(adl_operator_rvalue_started.handle);

  Task member_wins_over_adl_started =
      coroutine_member_operator_wins_over_adl();
  if (member_wins_over_adl_started.value != 0 ||
      member_wins_over_adl_started.handle == 0 ||
      frame_state(member_wins_over_adl_started.handle) != 1) {
    return 156;
  }
  resume_coroutine(member_wins_over_adl_started.handle);
  if (last_resume_value != 168 ||
      frame_state(member_wins_over_adl_started.handle) != 0 ||
      !frame_done(member_wins_over_adl_started.handle)) {
    return 157;
  }
  destroy_coroutine(member_wins_over_adl_started.handle);

  Task direct_await_local_started = coroutine_direct_await_keeps_local(40);
  if (direct_await_local_started.value != 0 ||
      direct_await_local_started.handle == 0 ||
      frame_state(direct_await_local_started.handle) != 1) {
    return 110;
  }
  resume_coroutine(direct_await_local_started.handle);
  if (last_resume_value != 60 ||
      frame_state(direct_await_local_started.handle) != 0 ||
      !frame_done(direct_await_local_started.handle)) {
    return 111;
  }
  destroy_coroutine(direct_await_local_started.handle);

  Task direct_await_loop_started = coroutine_direct_await_loop_expression();
  if (direct_await_loop_started.value != 0 ||
      direct_await_loop_started.handle == 0 ||
      frame_state(direct_await_loop_started.handle) != 1) {
    return 112;
  }
  resume_coroutine(direct_await_loop_started.handle);
  if (last_resume_value != 0 ||
      frame_state(direct_await_loop_started.handle) != 1 ||
      frame_done(direct_await_loop_started.handle)) {
    return 113;
  }
  resume_coroutine(direct_await_loop_started.handle);
  if (last_resume_value != 61 ||
      frame_state(direct_await_loop_started.handle) != 0 ||
      !frame_done(direct_await_loop_started.handle)) {
    return 114;
  }
  destroy_coroutine(direct_await_loop_started.handle);

  Task direct_await_factory_loop_started =
      coroutine_direct_await_factory_loop_expression();
  if (direct_await_factory_loop_started.value != 0 ||
      direct_await_factory_loop_started.handle == 0 ||
      frame_state(direct_await_factory_loop_started.handle) != 1) {
    return 121;
  }
  resume_coroutine(direct_await_factory_loop_started.handle);
  if (last_resume_value != 0 ||
      frame_state(direct_await_factory_loop_started.handle) != 1 ||
      frame_done(direct_await_factory_loop_started.handle)) {
    return 122;
  }
  resume_coroutine(direct_await_factory_loop_started.handle);
  if (last_resume_value != 81 ||
      frame_state(direct_await_factory_loop_started.handle) != 0 ||
      !frame_done(direct_await_factory_loop_started.handle)) {
    return 123;
  }
  destroy_coroutine(direct_await_factory_loop_started.handle);

  destroy_coroutine(started.handle);
  destroy_coroutine(yielded.handle);
  destroy_coroutine(initially_suspended.handle);
  if (promise_coroutine_operator_new_count != 51 ||
      promise_coroutine_operator_delete_count != 51 ||
      global_coroutine_operator_new_count != 2 ||
      global_coroutine_operator_delete_count != 2) {
    return 44;
  }

  return 0;
}
