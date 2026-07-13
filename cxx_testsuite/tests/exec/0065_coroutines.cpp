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

int frame_done(void* handle);
void resume_coroutine(void* handle);
void destroy_coroutine(void* handle);

namespace std {
template <class Promise>
struct coroutine_handle {
  void* handle;

  static coroutine_handle from_address(void* address) {
    coroutine_handle result = {address};
    return result;
  }

  static coroutine_handle from_promise(Promise& promise) {
    char* promise_address = (char*)&promise;
    unsigned long prefix_size = 2 * sizeof(int) + 2 * sizeof(void*);
    coroutine_handle result = {(void*)(promise_address - prefix_size)};
    return result;
  }

  void* address(void) const {
    return handle;
  }

  Promise& promise(void) const {
    char* frame_address = (char*)handle;
    unsigned long prefix_size = 2 * sizeof(int) + 2 * sizeof(void*);
    return *(Promise*)(frame_address + prefix_size);
  }

  bool done(void) const {
    return frame_done(handle);
  }

  void resume(void) const {
    resume_coroutine(handle);
  }

  void destroy(void) const {
    destroy_coroutine(handle);
  }
};
}

int last_resume_value;
int final_suspend_count;
int initial_suspend_count;
int coroutine_unhandled_exception_count;
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
int persisted_move_local_ctor_count;
int persisted_move_local_move_count;
int persisted_move_local_dtor_count;
int persisted_immovable_local_ctor_count;
int persisted_immovable_local_dtor_count;
int persisted_local_dtor_order;
int persisted_param_ctor_count;
int persisted_param_copy_count;
int persisted_param_move_count;
int persisted_param_dtor_count;
int member_coawait_temp_dtor_count;
int free_coawait_temp_dtor_count;
int noncopy_direct_awaiter_ctor_count;
int noncopy_direct_awaiter_dtor_count;
int move_only_awaiter_ctor_count;
int move_only_awaiter_move_count;
int move_only_awaiter_dtor_count;
int bool_await_suspend_count;
int transfer_await_suspend_count;
int transfer_target_result;
void* transfer_target_handle;
int condition_while_counter;
int condition_for_counter;
int condition_do_counter;
int condition_do_continue_counter;
int short_circuit_left_counter;
int short_circuit_right_counter;
int short_circuit_true_counter;
int short_circuit_false_counter;
int final_always_suspend_count;
int final_always_resume_count;
int final_always_dtor_count;
void* final_always_suspend_handle;
int await_transform_count;
int allocation_failure_operator_new_count;
int allocation_failure_operator_delete_count;
int allocation_failure_fallback_count;
int allocation_failure_next_new_fails;
int typed_handle_suspend_count;
int typed_handle_done_at_suspend;
void* typed_handle_address;
int from_promise_operator_new_count;
int from_promise_operator_delete_count;
int throwing_awaiter_ctor_count;
int throwing_awaiter_copy_count;
int throwing_awaiter_dtor_count;
unsigned long coroutine_frame_storage[1024];
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

struct ConditionSuspendValue {
  int* counter;
  int limit;
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    last_frame_handle = handle;
  }
  int await_resume(void) {
    *counter = *counter + 1;
    return *counter <= limit;
  }
};

struct CounterSuspendValue {
  int* counter;
  int value;
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    last_frame_handle = handle;
  }
  int await_resume(void) {
    *counter = *counter + 1;
    return value;
  }
};

struct TransformSuspendValue {
  int value;
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    last_frame_handle = handle;
  }
  int await_resume(void) {
    return value + 2;
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

struct TransferSuspendValue {
  int value;
  bool await_ready(void) {
    return false;
  }
  void* await_suspend(void* handle) {
    last_frame_handle = handle;
    transfer_await_suspend_count = transfer_await_suspend_count + 1;
    return transfer_target_handle;
  }
  int await_resume(void) {
    return value;
  }
};

struct HandleTransferSuspendValue {
  int value;
  bool await_ready(void) {
    return false;
  }
  std::coroutine_handle<Promise> await_suspend(void* handle) {
    last_frame_handle = handle;
    transfer_await_suspend_count = transfer_await_suspend_count + 1;
    return std::coroutine_handle<Promise>::from_address(transfer_target_handle);
  }
  int await_resume(void) {
    return value;
  }
};

struct TypedHandleSuspendValue {
  int value;
  bool await_ready(void) {
    return false;
  }
  void await_suspend(std::coroutine_handle<Promise> handle) {
    typed_handle_suspend_count = typed_handle_suspend_count + 1;
    typed_handle_address = handle.address();
    typed_handle_done_at_suspend = handle.done();
    last_frame_handle = handle.address();
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

struct NonCopyDirectSuspendValue {
  int value;
  NonCopyDirectSuspendValue(int input);
  NonCopyDirectSuspendValue(const NonCopyDirectSuspendValue& other) = delete;
  ~NonCopyDirectSuspendValue();
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

NonCopyDirectSuspendValue::NonCopyDirectSuspendValue(int input) {
  noncopy_direct_awaiter_ctor_count = noncopy_direct_awaiter_ctor_count + 1;
  value = input;
}

NonCopyDirectSuspendValue::~NonCopyDirectSuspendValue() {
  noncopy_direct_awaiter_dtor_count = noncopy_direct_awaiter_dtor_count + 1;
}

struct MoveOnlyNamedSuspendValue {
  int value;
  MoveOnlyNamedSuspendValue(int input);
  MoveOnlyNamedSuspendValue(const MoveOnlyNamedSuspendValue& other) = delete;
  MoveOnlyNamedSuspendValue(MoveOnlyNamedSuspendValue&& other);
  ~MoveOnlyNamedSuspendValue();
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

MoveOnlyNamedSuspendValue::MoveOnlyNamedSuspendValue(int input) {
  move_only_awaiter_ctor_count = move_only_awaiter_ctor_count + 1;
  value = input;
}

MoveOnlyNamedSuspendValue::MoveOnlyNamedSuspendValue(
    MoveOnlyNamedSuspendValue&& other) {
  move_only_awaiter_move_count = move_only_awaiter_move_count + 1;
  value = other.value + 400;
}

MoveOnlyNamedSuspendValue::~MoveOnlyNamedSuspendValue() {
  move_only_awaiter_dtor_count = move_only_awaiter_dtor_count + 1;
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

struct ThrowingTrackedAwaiter {
  int value;
  ThrowingTrackedAwaiter(int input);
  ThrowingTrackedAwaiter(const ThrowingTrackedAwaiter& other);
  ~ThrowingTrackedAwaiter();
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    last_frame_handle = handle;
  }
  int await_resume(void) {
    (void)value;
    throw 23;
  }
};

ThrowingTrackedAwaiter::ThrowingTrackedAwaiter(int input) {
  throwing_awaiter_ctor_count = throwing_awaiter_ctor_count + 1;
  value = input;
}

ThrowingTrackedAwaiter::ThrowingTrackedAwaiter(
    const ThrowingTrackedAwaiter& other) {
  throwing_awaiter_copy_count = throwing_awaiter_copy_count + 1;
  value = other.value;
}

ThrowingTrackedAwaiter::~ThrowingTrackedAwaiter() {
  throwing_awaiter_dtor_count = throwing_awaiter_dtor_count + 1;
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

struct PersistedMoveOnlyLocalBox {
  int value;
  PersistedMoveOnlyLocalBox(int input);
  PersistedMoveOnlyLocalBox(const PersistedMoveOnlyLocalBox& other) = delete;
  PersistedMoveOnlyLocalBox(PersistedMoveOnlyLocalBox&& other);
  ~PersistedMoveOnlyLocalBox();
};

PersistedMoveOnlyLocalBox::PersistedMoveOnlyLocalBox(int input) {
  persisted_move_local_ctor_count = persisted_move_local_ctor_count + 1;
  value = input;
}

PersistedMoveOnlyLocalBox::PersistedMoveOnlyLocalBox(
    PersistedMoveOnlyLocalBox&& other) {
  persisted_move_local_move_count = persisted_move_local_move_count + 1;
  value = other.value + 300;
}

PersistedMoveOnlyLocalBox::~PersistedMoveOnlyLocalBox() {
  persisted_move_local_dtor_count = persisted_move_local_dtor_count + 1;
}

struct PersistedImmovableLocalBox {
  int value;
  PersistedImmovableLocalBox(int input);
  PersistedImmovableLocalBox(const PersistedImmovableLocalBox& other) = delete;
  PersistedImmovableLocalBox(PersistedImmovableLocalBox&& other) = delete;
  ~PersistedImmovableLocalBox();
};

PersistedImmovableLocalBox::PersistedImmovableLocalBox(int input) {
  persisted_immovable_local_ctor_count =
      persisted_immovable_local_ctor_count + 1;
  value = input;
}

PersistedImmovableLocalBox::~PersistedImmovableLocalBox() {
  persisted_immovable_local_dtor_count =
      persisted_immovable_local_dtor_count + 1;
}

struct PersistedOrderLocal {
  int id;
  PersistedOrderLocal(int input);
  PersistedOrderLocal(const PersistedOrderLocal& other) = delete;
  PersistedOrderLocal(PersistedOrderLocal&& other) = delete;
  ~PersistedOrderLocal();
};

PersistedOrderLocal::PersistedOrderLocal(int input) {
  id = input;
}

PersistedOrderLocal::~PersistedOrderLocal() {
  persisted_local_dtor_order = persisted_local_dtor_order * 10 + id;
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
    if (result == 7777) {
      throw 31;
    }
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
    coroutine_unhandled_exception_count =
        coroutine_unhandled_exception_count + 1;
    value = -909;
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

struct YieldExceptionPromise;

struct YieldExceptionTask {
  using promise_type = YieldExceptionPromise;
  int value;
  void* handle;
};

struct YieldExceptionInitialSuspend {
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    last_frame_handle = handle;
  }
  void await_resume(void) {
  }
};

struct YieldExceptionPromise {
  int value;
  void* operator new(unsigned long size);
  void operator delete(void* ptr);

  YieldExceptionTask get_return_object(void) {
    std::coroutine_handle<YieldExceptionPromise> handle =
        std::coroutine_handle<YieldExceptionPromise>::from_promise(*this);
    YieldExceptionTask task = {value, handle.address()};
    return task;
  }

  YieldExceptionInitialSuspend initial_suspend(void) {
    YieldExceptionInitialSuspend value = {};
    return value;
  }

  SuspendNever final_suspend(void) {
    SuspendNever value = {};
    return value;
  }

  void return_value(int result) {
    value = result;
  }

  ThrowingTrackedAwaiter yield_value(int result) {
    if (result == 881) {
      throw 41;
    }
    value = result;
    ThrowingTrackedAwaiter awaiter(result);
    return awaiter;
  }

  void unhandled_exception(void) {
    coroutine_unhandled_exception_count =
        coroutine_unhandled_exception_count + 1;
    value = -929;
  }
};

void* YieldExceptionPromise::operator new(unsigned long size) {
  int words = (int)((size + 7) / 8);
  void* result = &coroutine_frame_storage[coroutine_frame_storage_index];
  coroutine_frame_storage_index = coroutine_frame_storage_index + words;
  promise_coroutine_operator_new_count =
      promise_coroutine_operator_new_count + 1;
  return result;
}

void YieldExceptionPromise::operator delete(void* ptr) {
  (void)ptr;
  promise_coroutine_operator_delete_count =
      promise_coroutine_operator_delete_count + 1;
}

struct TransformPromise;

struct TransformTask {
  using promise_type = TransformPromise;
  int value;
  void* handle;
};

struct TransformPromise {
  int value;
  void* operator new(unsigned long size);
  void operator delete(void* ptr);

  TransformTask get_return_object(void) {
    TransformTask task = {value, last_frame_handle};
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

  TransformSuspendValue await_transform(int input) {
    await_transform_count = await_transform_count + 1;
    TransformSuspendValue awaiter = {input + 5};
    return awaiter;
  }

  void unhandled_exception(void) {
  }
};

void* TransformPromise::operator new(unsigned long size) {
  int words = (int)((size + 7) / 8);
  void* result = &coroutine_frame_storage[coroutine_frame_storage_index];
  coroutine_frame_storage_index = coroutine_frame_storage_index + words;
  promise_coroutine_operator_new_count =
      promise_coroutine_operator_new_count + 1;
  return result;
}

void TransformPromise::operator delete(void* ptr) {
  (void)ptr;
  promise_coroutine_operator_delete_count =
      promise_coroutine_operator_delete_count + 1;
}

struct AllocationFailurePromise;

struct AllocationFailureTask {
  using promise_type = AllocationFailurePromise;
  int value;
  void* handle;
};

struct AllocationFailurePromise {
  int value;
  void* operator new(unsigned long size);
  void operator delete(void* ptr);

  static AllocationFailureTask get_return_object_on_allocation_failure(void) {
    allocation_failure_fallback_count =
        allocation_failure_fallback_count + 1;
    AllocationFailureTask task = {-777, 0};
    return task;
  }

  AllocationFailureTask get_return_object(void) {
    AllocationFailureTask task = {value, last_frame_handle};
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

  void unhandled_exception(void) {
  }
};

void* AllocationFailurePromise::operator new(unsigned long size) {
  allocation_failure_operator_new_count =
      allocation_failure_operator_new_count + 1;
  if (allocation_failure_next_new_fails) {
    allocation_failure_next_new_fails = 0;
    return 0;
  }
  int words = (int)((size + 7) / 8);
  void* result = &coroutine_frame_storage[coroutine_frame_storage_index];
  coroutine_frame_storage_index = coroutine_frame_storage_index + words;
  return result;
}

void AllocationFailurePromise::operator delete(void* ptr) {
  (void)ptr;
  allocation_failure_operator_delete_count =
      allocation_failure_operator_delete_count + 1;
}

struct FromPromisePromise;

struct FromPromiseTask {
  using promise_type = FromPromisePromise;
  int value;
  void* handle;
};

struct FromPromisePromise {
  int value;
  void* operator new(unsigned long size);
  void operator delete(void* ptr);

  FromPromiseTask get_return_object(void) {
    std::coroutine_handle<FromPromisePromise> handle =
        std::coroutine_handle<FromPromisePromise>::from_promise(*this);
    FromPromiseTask task = {value, handle.address()};
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

  void unhandled_exception(void) {
  }
};

void* FromPromisePromise::operator new(unsigned long size) {
  from_promise_operator_new_count = from_promise_operator_new_count + 1;
  int words = (int)((size + 7) / 8);
  void* result = &coroutine_frame_storage[coroutine_frame_storage_index];
  coroutine_frame_storage_index = coroutine_frame_storage_index + words;
  return result;
}

void FromPromisePromise::operator delete(void* ptr) {
  (void)ptr;
  from_promise_operator_delete_count = from_promise_operator_delete_count + 1;
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

TransformTask coroutine_await_transform_int(void) {
  int value = co_await 300;
  co_return value + 7;
}

AllocationFailureTask coroutine_allocation_failure_value(void) {
  SuspendOnce awaiter = {};
  int value = co_await awaiter;
  co_return value + 643;
}

FromPromiseTask coroutine_from_promise_handle(void) {
  SuspendOnce awaiter = {};
  int value = co_await awaiter;
  co_return value + 900;
}

Task coroutine_suspend_once(void) {
  SuspendOnce awaiter = {};
  int value = co_await awaiter;
  co_return value + 2;
}

Task coroutine_throw_after_suspend(void) {
  SuspendOnce awaiter = {};
  co_await awaiter;
  throw 17;
}

void coroutine_throw_helper(void) {
  throw 19;
}

Task coroutine_indirect_throw_after_suspend(void) {
  SuspendOnce awaiter = {};
  co_await awaiter;
  coroutine_throw_helper();
  co_return 1;
}

Task coroutine_throw_cleans_persisted_local(void) {
  PersistedLocalBox kept(52);
  SuspendOnce awaiter = {};
  co_await awaiter;
  coroutine_throw_helper();
  co_return kept.value;
}

Task coroutine_await_resume_throw_cleans_awaiter(void) {
  ThrowingTrackedAwaiter awaiter(111);
  int value = co_await awaiter;
  co_return value;
}

YieldExceptionTask coroutine_yield_value_throws(void) {
  co_yield 881;
  co_return 1;
}

YieldExceptionTask coroutine_yield_await_resume_throw_cleans_awaiter(void) {
  co_yield 882;
  co_return 2;
}

Task coroutine_return_value_throw_after_suspend(void) {
  SuspendOnce awaiter = {};
  co_await awaiter;
  co_return 7777;
}

Task coroutine_nested_try_catches_throw(void) {
  SuspendOnce awaiter = {};
  co_await awaiter;
  try {
    coroutine_throw_helper();
  } catch (int value) {
    co_return value + 302;
  }
  co_return 1;
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

Task coroutine_typed_handle_await_suspend(void) {
  TypedHandleSuspendValue awaiter = {421};
  int value = co_await awaiter;
  co_return value + 8;
}

Task coroutine_transfer_target(void) {
  SuspendValue awaiter = {211};
  int value = co_await awaiter;
  transfer_target_result = value + 5;
  co_return transfer_target_result;
}

Task coroutine_transfer_source(void) {
  TransferSuspendValue awaiter = {307};
  int value = co_await awaiter;
  co_return value + 6;
}

Task coroutine_handle_transfer_source(void) {
  HandleTransferSuspendValue awaiter = {317};
  int value = co_await awaiter;
  co_return value + 6;
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

Task coroutine_direct_await_noncopyable_expression(void) {
  int value = co_await NonCopyDirectSuspendValue{29};
  co_return value + 4;
}

Task coroutine_direct_await_noncopyable_loop_expression(void) {
  int sum = 0;
  int i = 0;
  while (i < 2) {
    int value = co_await NonCopyDirectSuspendValue{50 + i};
    sum = sum + value;
    i = i + 1;
  }
  co_return sum;
}

Task coroutine_co_return_direct_noncopyable_co_await(void) {
  co_return co_await NonCopyDirectSuspendValue{63};
}

Task coroutine_direct_await_aggregate_expression(void) {
  int value = co_await SuspendValue{23};
  co_return value + 4;
}

Task coroutine_direct_await_factory_expression(void) {
  int value = co_await make_suspend_value(25);
  co_return value + 4;
}

Task coroutine_co_await_statement_named(void) {
  SuspendValue awaiter = {171};
  co_await awaiter;
  co_return 17;
}

Task coroutine_co_await_statement_temporary(void) {
  co_await DirectSuspendValue{181};
  co_return 19;
}

Task coroutine_co_await_assignment_rhs(void) {
  SuspendValue awaiter = {191};
  int value = 0;
  value = co_await awaiter;
  co_return value + 3;
}

Task coroutine_co_await_parameter(SuspendValue awaiter) {
  int value = co_await awaiter;
  co_return value + 5;
}

Task coroutine_move_only_named_awaiter(void) {
  MoveOnlyNamedSuspendValue awaiter(251);
  int value = co_await awaiter;
  co_return value + 5;
}

Task coroutine_co_return_co_await(void) {
  SuspendValue awaiter = {197};
  co_return co_await awaiter;
}

Task coroutine_co_await_binary_initializer(void) {
  SuspendValue awaiter = {201};
  int value = 7 + co_await awaiter;
  co_return value + 3;
}

Task coroutine_co_await_multiple_binary_initializer(void) {
  SuspendValue first = {227};
  SuspendValue second = {229};
  int value = (co_await first) + (co_await second);
  co_return value + 8;
}

Task coroutine_co_await_logand_right_skipped(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int value = co_await CounterSuspendValue{&short_circuit_left_counter, 0} &&
              co_await CounterSuspendValue{&short_circuit_right_counter, 1};
  co_return value + 700;
}

Task coroutine_co_await_logand_right_taken(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int value = co_await CounterSuspendValue{&short_circuit_left_counter, 1} &&
              co_await CounterSuspendValue{&short_circuit_right_counter, 1};
  co_return value + 710;
}

Task coroutine_co_await_logor_right_skipped(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int value = co_await CounterSuspendValue{&short_circuit_left_counter, 1} ||
              co_await CounterSuspendValue{&short_circuit_right_counter, 0};
  co_return value + 720;
}

Task coroutine_co_await_logor_right_taken(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int value = co_await CounterSuspendValue{&short_circuit_left_counter, 0} ||
              co_await CounterSuspendValue{&short_circuit_right_counter, 1};
  co_return value + 730;
}

Task coroutine_co_await_conditional_true_arm(void) {
  short_circuit_left_counter = 0;
  short_circuit_true_counter = 0;
  short_circuit_false_counter = 0;
  int value = co_await CounterSuspendValue{&short_circuit_left_counter, 1}
                  ? co_await CounterSuspendValue{&short_circuit_true_counter,
                                                 307}
                  : co_await CounterSuspendValue{&short_circuit_false_counter,
                                                 409};
  co_return value + 10;
}

Task coroutine_co_await_conditional_false_arm(void) {
  short_circuit_left_counter = 0;
  short_circuit_true_counter = 0;
  short_circuit_false_counter = 0;
  int value = co_await CounterSuspendValue{&short_circuit_left_counter, 0}
                  ? co_await CounterSuspendValue{&short_circuit_true_counter,
                                                 307}
                  : co_await CounterSuspendValue{&short_circuit_false_counter,
                                                 409};
  co_return value + 10;
}

Task coroutine_co_await_comma_sequence(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int value = ((co_await CounterSuspendValue{&short_circuit_left_counter, 303}),
               (co_await CounterSuspendValue{&short_circuit_right_counter, 407}));
  co_return value + 11;
}

Task coroutine_co_await_binary_assignment_rhs(void) {
  SuspendValue awaiter = {211};
  int value = 0;
  value = 9 + co_await awaiter;
  co_return value + 4;
}

int add_coroutine_values(int left, int middle, int right) {
  return left + middle + right;
}

Task coroutine_co_await_call_argument(void) {
  SuspendValue awaiter = {223};
  int value = add_coroutine_values(5, co_await awaiter, 6);
  co_return value + 7;
}

Task coroutine_co_await_if_condition(void) {
  SuspendValue awaiter = {1};
  int value = 0;
  if (co_await awaiter) {
    value = 271;
  } else {
    value = 11;
  }
  co_return value + 2;
}

Task coroutine_co_await_while_condition(void) {
  condition_while_counter = 0;
  int sum = 0;
  while (co_await ConditionSuspendValue{&condition_while_counter, 2}) {
    sum = sum + condition_while_counter;
  }
  co_return sum + 300;
}

Task coroutine_co_await_for_condition(void) {
  condition_for_counter = 0;
  int sum = 0;
  for (; co_await ConditionSuspendValue{&condition_for_counter, 2}; ) {
    sum = sum + condition_for_counter * 2;
  }
  co_return sum + 400;
}

Task coroutine_co_await_do_condition(void) {
  condition_do_counter = 0;
  int sum = 0;
  do {
    sum = sum + condition_do_counter;
  } while (co_await ConditionSuspendValue{&condition_do_counter, 2});
  co_return sum + 500;
}

Task coroutine_co_await_do_condition_continue(void) {
  condition_do_continue_counter = 0;
  int sum = 0;
  do {
    sum = sum + 10;
    if (condition_do_continue_counter < 2) {
      continue;
    }
    sum = sum + 100;
  } while (co_await ConditionSuspendValue{&condition_do_continue_counter, 2});
  co_return sum + 600;
}

Task coroutine_co_await_while_short_circuit_condition(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int sum = 0;
  while (co_await ConditionSuspendValue{&short_circuit_left_counter, 2} &&
         co_await ConditionSuspendValue{&short_circuit_right_counter, 10}) {
    sum = sum + short_circuit_left_counter * 10 +
          short_circuit_right_counter;
  }
  co_return 800 + sum + short_circuit_left_counter * 100 +
            short_circuit_right_counter * 10;
}

Task coroutine_co_await_for_short_circuit_condition(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int body_count = 0;
  for (; co_await ConditionSuspendValue{&short_circuit_left_counter, 2} ||
         co_await ConditionSuspendValue{&short_circuit_right_counter, 1}; ) {
    body_count = body_count + 1;
  }
  co_return 900 + body_count * 100 + short_circuit_left_counter * 10 +
            short_circuit_right_counter;
}

Task coroutine_co_await_do_short_circuit_continue_condition(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int sum = 0;
  do {
    sum = sum + 10;
    if (short_circuit_left_counter < 2) {
      continue;
    }
    sum = sum + 100;
  } while (co_await ConditionSuspendValue{&short_circuit_left_counter, 2} &&
           co_await ConditionSuspendValue{&short_circuit_right_counter, 10});
  co_return 1400 + sum + short_circuit_left_counter * 10 +
            short_circuit_right_counter;
}

Task coroutine_co_await_switch_condition(void) {
  short_circuit_left_counter = 0;
  int selected = 0;
  switch (co_await CounterSuspendValue{&short_circuit_left_counter, 2}) {
    case 1:
      selected = 1;
      break;
    case 2:
      selected = 2;
      break;
    default:
      selected = 9;
      break;
  }
  co_return 1000 + selected + short_circuit_left_counter * 10;
}

Task coroutine_co_await_switch_binary_condition(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int selected = 0;
  switch ((co_await CounterSuspendValue{&short_circuit_left_counter, 3}) +
          (co_await CounterSuspendValue{&short_circuit_right_counter, 4})) {
    case 7:
      selected = 7;
      break;
    default:
      selected = 9;
      break;
  }
  co_return 1100 + selected + short_circuit_left_counter * 10 +
            short_circuit_right_counter;
}

Task coroutine_co_await_switch_conditional_condition(void) {
  short_circuit_left_counter = 0;
  short_circuit_true_counter = 0;
  short_circuit_false_counter = 0;
  int selected = 0;
  switch (co_await CounterSuspendValue{&short_circuit_left_counter, 0}
              ? co_await CounterSuspendValue{&short_circuit_true_counter, 3}
              : co_await CounterSuspendValue{&short_circuit_false_counter, 4}) {
    case 3:
      selected = 3;
      break;
    case 4:
      selected = 4;
      break;
    default:
      selected = 9;
      break;
  }
  co_return 1200 + selected + short_circuit_left_counter * 100 +
            short_circuit_true_counter * 10 + short_circuit_false_counter;
}

Task coroutine_co_await_nested_try_and_label(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int value = 0;
  try {
    if (co_await CounterSuspendValue{&short_circuit_left_counter, 1})
      value = value + co_await CounterSuspendValue{&short_circuit_right_counter, 2};
  } catch (...) {
    value = 90;
  }
  if (value == -1)
    goto after_try;
after_try:
  value = value + co_await CounterSuspendValue{&short_circuit_true_counter, 3};
  co_return 1300 + value + short_circuit_left_counter * 100 +
            short_circuit_right_counter * 10 + short_circuit_true_counter;
}

Task coroutine_co_await_nested_switch_case_body(void) {
  short_circuit_left_counter = 0;
  int value = 0;
  switch (1) {
    case 1:
      value = co_await CounterSuspendValue{&short_circuit_left_counter, 4};
      break;
    default:
      value = 9;
      break;
  }
  co_return 1400 + value + short_circuit_left_counter * 10;
}

Task coroutine_co_await_nested_catch_body(void) {
  short_circuit_left_counter = 0;
  int value = 0;
  try {
    coroutine_throw_helper();
  } catch (int caught) {
    value = caught + co_await CounterSuspendValue{&short_circuit_left_counter, 5};
  }
  co_return 1500 + value + short_circuit_left_counter * 10;
}

Task coroutine_co_await_catch_parameter_multiple_suspensions(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int value = 0;
  try {
    coroutine_throw_helper();
  } catch (int caught) {
    int first = co_await CounterSuspendValue{&short_circuit_left_counter, 5};
    value = caught + first;
    int second = co_await CounterSuspendValue{&short_circuit_right_counter, 6};
    value = value + caught + second;
  }
  co_return 1600 + value + short_circuit_left_counter * 100 +
            short_circuit_right_counter * 10;
}

Task coroutine_co_await_nested_catch_parameter_control_flow(void) {
  short_circuit_left_counter = 0;
  int value = 0;
  try {
    coroutine_throw_helper();
  } catch (int caught) {
    if (caught == 19) {
      switch (caught) {
        case 19:
          value = caught + co_await CounterSuspendValue{&short_circuit_left_counter, 7};
          break;
        default:
          value = 90;
          break;
      }
    }
  }
  co_return 1700 + value + short_circuit_left_counter * 10;
}

Task coroutine_co_return_binary_co_await(void) {
  SuspendValue awaiter = {233};
  co_return 11 + co_await awaiter;
}

Task coroutine_co_return_multiple_binary_co_await(void) {
  SuspendValue first = {239};
  SuspendValue second = {241};
  co_return (co_await first) + (co_await second) + 10;
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

Task coroutine_co_yield_declaration_result(void) {
  int value = co_yield 111;
  co_return value + 5;
}

Task coroutine_co_yield_assignment_result(void) {
  int value = 1;
  value = value + (co_yield 121);
  co_return value + 5;
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
    coroutine_unhandled_exception_count =
        coroutine_unhandled_exception_count + 1;
    value = -919;
  }
};

InitialTask coroutine_initially_suspended(void) {
  initial_suspend_body_count = initial_suspend_body_count + 1;
  co_return 404;
}

InitialTask coroutine_initial_throw_before_body_suspend(void) {
  initial_suspend_body_count = initial_suspend_body_count + 1;
  throw 41;
  co_return 0;
}

struct FinalSuspendAlways {
  ~FinalSuspendAlways();
  bool await_ready(void) {
    return false;
  }
  void await_suspend(void* handle) {
    final_always_suspend_count = final_always_suspend_count + 1;
    final_always_suspend_handle = handle;
    last_frame_handle = handle;
  }
  void await_resume(void) {
    final_always_resume_count = final_always_resume_count + 1;
  }
};

FinalSuspendAlways::~FinalSuspendAlways() {
  final_always_dtor_count = final_always_dtor_count + 1;
}

struct FinalSuspendTask;
typedef struct FinalSuspendTask FinalSuspendTask;
struct FinalSuspendPromise;

struct FinalSuspendTask {
  using promise_type = FinalSuspendPromise;
  int value;
  void* handle;
};

struct FinalSuspendPromise {
  int value;
  FinalSuspendTask get_return_object(void) {
    FinalSuspendTask task = {value, last_frame_handle};
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

FinalSuspendTask coroutine_finally_suspended(void) {
  co_return 515;
}

struct FinalTransferPromise;

struct FinalTransferTask {
  using promise_type = FinalTransferPromise;
  int value;
  void* handle;
};

struct FinalTransferPromise {
  int value;
  void* operator new(unsigned long size);
  void operator delete(void* ptr);

  FinalTransferTask get_return_object(void) {
    std::coroutine_handle<FinalTransferPromise> handle =
        std::coroutine_handle<FinalTransferPromise>::from_promise(*this);
    FinalTransferTask task = {value, handle.address()};
    return task;
  }

  SuspendNever initial_suspend(void) {
    SuspendNever awaiter = {};
    return awaiter;
  }

  TransferSuspendValue final_suspend(void) {
    TransferSuspendValue awaiter = {0};
    return awaiter;
  }

  void return_value(int result) {
    value = result;
  }

  void unhandled_exception(void) {
    value = -939;
  }
};

void* FinalTransferPromise::operator new(unsigned long size) {
  int words = (int)((size + 7) / 8);
  void* result = &coroutine_frame_storage[coroutine_frame_storage_index];
  coroutine_frame_storage_index = coroutine_frame_storage_index + words;
  promise_coroutine_operator_new_count =
      promise_coroutine_operator_new_count + 1;
  return result;
}

void FinalTransferPromise::operator delete(void* ptr) {
  (void)ptr;
  promise_coroutine_operator_delete_count =
      promise_coroutine_operator_delete_count + 1;
}

FinalTransferTask coroutine_final_suspend_transfer_source(void) {
  co_return 123;
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

Task coroutine_persist_move_only_class_local_suspend(void) {
  PersistedMoveOnlyLocalBox kept(70);
  SuspendValue awaiter = {8};
  int value = co_await awaiter;
  co_return kept.value + value;
}

Task coroutine_persist_immovable_class_local_suspend(void) {
  PersistedImmovableLocalBox kept(80);
  SuspendValue awaiter = {9};
  int value = co_await awaiter;
  co_return kept.value + value;
}

Task coroutine_nested_persisted_local_destruction_order(void) {
  PersistedOrderLocal outer(1);
  {
    PersistedOrderLocal inner(2);
    SuspendValue awaiter = {4};
    int value = co_await awaiter;
    co_return outer.id + inner.id + value;
  }
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

Task coroutine_co_yield_if_condition(void) {
  int value = 0;
  if (co_yield 911) {
    value = 1;
  }
  co_return 912 + value;
}

Task coroutine_co_yield_while_condition(void) {
  int i = 0;
  int sum = 0;
  while (co_yield (i < 2)) {
    sum = sum + i + 1;
    i = i + 1;
  }
  co_return 920 + sum;
}

Task coroutine_co_yield_do_condition_continue(void) {
  int i = 0;
  int sum = 0;
  do {
    i = i + 1;
    if (i < 2) {
      continue;
    }
    sum = sum + 10;
  } while (co_yield (i < 2));
  co_return 930 + sum + i;
}

Task coroutine_co_yield_switch_condition(void) {
  int selected = 0;
  switch (co_yield 2) {
    case 1:
      selected = 1;
      break;
    case 2:
      selected = 2;
      break;
    default:
      selected = 9;
      break;
  }
  co_return 940 + selected;
}

Task coroutine_co_yield_short_circuit_condition(void) {
  int value = (co_yield 1) && (co_yield 0);
  co_return 950 + value;
}

Task coroutine_co_yield_conditional_expression(void) {
  int value = (co_yield 1) ? (co_yield 5) : (co_yield 7);
  co_return 960 + value;
}

Task coroutine_mixed_co_await_co_yield_logand(void) {
  short_circuit_left_counter = 0;
  int value = (co_await CounterSuspendValue{&short_circuit_left_counter, 1}) &&
              (co_yield 1);
  co_return 970 + value;
}

Task coroutine_mixed_co_await_co_yield_logor_skip(void) {
  short_circuit_left_counter = 0;
  short_circuit_right_counter = 0;
  int value = (co_await CounterSuspendValue{&short_circuit_left_counter, 1}) ||
              (co_yield 9);
  co_return 980 + value + short_circuit_left_counter * 10 +
            short_circuit_right_counter;
}

Task coroutine_mixed_co_await_co_yield_conditional(void) {
  short_circuit_left_counter = 0;
  int value = (co_await CounterSuspendValue{&short_circuit_left_counter, 0})
                  ? (co_yield 5)
                  : (co_yield 7);
  co_return 990 + value;
}

Task coroutine_mixed_co_yield_co_await_comma(void) {
  short_circuit_right_counter = 0;
  int value = ((co_yield 4),
               (co_await CounterSuspendValue{&short_circuit_right_counter, 6}));
  co_return 1000 + value;
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

  int unhandled_exception_before = coroutine_unhandled_exception_count;
  Task throw_started = coroutine_throw_after_suspend();
  if (throw_started.value != 0 || throw_started.handle == 0 ||
      frame_state(throw_started.handle) != 1 ||
      frame_done(throw_started.handle)) {
    return 206;
  }
  resume_coroutine(throw_started.handle);
  if (last_resume_value != -909 ||
      coroutine_unhandled_exception_count != unhandled_exception_before + 1 ||
      frame_state(throw_started.handle) != 0 ||
      !frame_done(throw_started.handle)) {
    return 207;
  }
  destroy_coroutine(throw_started.handle);

  Task indirect_throw_started = coroutine_indirect_throw_after_suspend();
  if (indirect_throw_started.value != 0 ||
      indirect_throw_started.handle == 0 ||
      frame_state(indirect_throw_started.handle) != 1 ||
      frame_done(indirect_throw_started.handle)) {
    return 208;
  }
  resume_coroutine(indirect_throw_started.handle);
  if (last_resume_value != -909 ||
      coroutine_unhandled_exception_count != unhandled_exception_before + 2 ||
      frame_state(indirect_throw_started.handle) != 0 ||
      !frame_done(indirect_throw_started.handle)) {
    return 209;
  }
  destroy_coroutine(indirect_throw_started.handle);

  int persisted_local_dtor_before_exception = persisted_local_dtor_count;
  Task local_throw_started = coroutine_throw_cleans_persisted_local();
  if (local_throw_started.value != 0 || local_throw_started.handle == 0 ||
      frame_state(local_throw_started.handle) != 1 ||
      frame_done(local_throw_started.handle)) {
    return 210;
  }
  resume_coroutine(local_throw_started.handle);
  if (last_resume_value != -909 ||
      coroutine_unhandled_exception_count != unhandled_exception_before + 3 ||
      persisted_local_dtor_count != persisted_local_dtor_before_exception + 1 ||
      frame_state(local_throw_started.handle) != 0 ||
      !frame_done(local_throw_started.handle)) {
    return 211;
  }
  destroy_coroutine(local_throw_started.handle);
  if (persisted_local_dtor_count != persisted_local_dtor_before_exception + 1) {
    return 212;
  }

  int throwing_awaiter_dtor_before_exception = throwing_awaiter_dtor_count;
  Task await_resume_throw_started =
      coroutine_await_resume_throw_cleans_awaiter();
  if (throwing_awaiter_ctor_count != 1 ||
      throwing_awaiter_copy_count != 1 ||
      await_resume_throw_started.value != 0 ||
      await_resume_throw_started.handle == 0 ||
      frame_state(await_resume_throw_started.handle) != 1 ||
      frame_done(await_resume_throw_started.handle)) {
    return 213;
  }
  resume_coroutine(await_resume_throw_started.handle);
  if (last_resume_value != -909 ||
      coroutine_unhandled_exception_count != unhandled_exception_before + 4 ||
      throwing_awaiter_dtor_count !=
          throwing_awaiter_dtor_before_exception + 1 ||
      frame_state(await_resume_throw_started.handle) != 0 ||
      !frame_done(await_resume_throw_started.handle)) {
    return 214;
  }
  destroy_coroutine(await_resume_throw_started.handle);
  if (throwing_awaiter_dtor_count !=
      throwing_awaiter_dtor_before_exception + 1) {
    return 215;
  }

  Task return_value_throw_started = coroutine_return_value_throw_after_suspend();
  if (return_value_throw_started.value != 0 ||
      return_value_throw_started.handle == 0 ||
      frame_state(return_value_throw_started.handle) != 1 ||
      frame_done(return_value_throw_started.handle)) {
    return 216;
  }
  resume_coroutine(return_value_throw_started.handle);
  if (last_resume_value != -909 ||
      coroutine_unhandled_exception_count != unhandled_exception_before + 5 ||
      frame_state(return_value_throw_started.handle) != 0 ||
      !frame_done(return_value_throw_started.handle)) {
    return 217;
  }
  destroy_coroutine(return_value_throw_started.handle);

  Task nested_try_started = coroutine_nested_try_catches_throw();
  if (nested_try_started.value != 0 || nested_try_started.handle == 0 ||
      frame_state(nested_try_started.handle) != 1 ||
      frame_done(nested_try_started.handle)) {
    return 218;
  }
  resume_coroutine(nested_try_started.handle);
  if (last_resume_value != 321) {
    return 219;
  }
  if (coroutine_unhandled_exception_count != unhandled_exception_before + 5) {
    return 222;
  }
  if (frame_state(nested_try_started.handle) != 0 ||
      !frame_done(nested_try_started.handle)) {
    return 223;
  }
  destroy_coroutine(nested_try_started.handle);

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
  if (final_suspend_count != 13) {
    return 12;
  }
  CoroutineFrame* completed_twice_frame = (CoroutineFrame*)last_frame_handle;
  if (completed_twice_frame->resume != 0 ||
      completed_twice_frame->destroy == 0) {
    return 28;
  }

  resume_coroutine(last_frame_handle);
  if (last_resume_value != -1 || final_suspend_count != 13) {
    return 13;
  }
  destroy_coroutine(last_frame_handle);
  if (frame_state(last_frame_handle) != 0 || !frame_done(last_frame_handle) ||
      completed_twice_frame->resume != 0 ||
      completed_twice_frame->destroy != 0 || final_suspend_count != 13) {
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
      final_suspend_count != 13) {
    return 26;
  }
  destroy_coroutine(destroy_started.handle);
  if (last_resume_value != -1 || suspend_twice_start_count != 2 ||
      final_suspend_count != 13) {
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
      !frame_done(handle_b) || final_suspend_count != 15) {
    return 42;
  }
  destroy_coroutine(handle_a);
  destroy_coroutine(handle_b);
  if (promise_coroutine_operator_new_count != 13 ||
      promise_coroutine_operator_delete_count != 12 ||
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
  if (final_suspend_count != 16) {
    return 17;
  }

  resume_yield_coroutine(last_frame_handle);
  if (last_resume_value != -1 || final_suspend_count != 16) {
    return 18;
  }
  if (initial_suspend_count != 17) {
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
      final_suspend_count != 17) {
    return 35;
  }

  resume_coroutine(initially_suspended.handle);
  if (last_resume_value != -1 || initial_suspend_body_count != 1 ||
      initial_always_resume_count != 1 || final_suspend_count != 17) {
    return 36;
  }

  Task yield_decl_started = coroutine_co_yield_declaration_result();
  if (yield_decl_started.value != 111 || yield_decl_started.handle == 0 ||
      frame_state(yield_decl_started.handle) != 1 ||
      frame_done(yield_decl_started.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_decl_started.handle);
  if (last_resume_value != 116 ||
      frame_state(yield_decl_started.handle) != 0 ||
      !frame_done(yield_decl_started.handle)) {
    return 254;
  }
  destroy_coroutine(yield_decl_started.handle);

  Task yield_assign_started = coroutine_co_yield_assignment_result();
  if (yield_assign_started.value != 121 || yield_assign_started.handle == 0 ||
      frame_state(yield_assign_started.handle) != 1 ||
      frame_done(yield_assign_started.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_assign_started.handle);
  if (last_resume_value != 127 ||
      frame_state(yield_assign_started.handle) != 0 ||
      !frame_done(yield_assign_started.handle)) {
    return 254;
  }
  destroy_coroutine(yield_assign_started.handle);

  last_frame_handle = 0;
  InitialTask initial_throw_started =
      coroutine_initial_throw_before_body_suspend();
  if (initial_throw_started.value != 0 || initial_throw_started.handle == 0 ||
      initial_throw_started.handle != last_frame_handle ||
      frame_state(initial_throw_started.handle) != 0 ||
      frame_done(initial_throw_started.handle)) {
    return 220;
  }
  resume_coroutine(initial_throw_started.handle);
  if (last_resume_value != -919 ||
      coroutine_unhandled_exception_count != unhandled_exception_before + 6 ||
      initial_always_suspend_count != 2 ||
      initial_always_resume_count != 2 ||
      initial_suspend_body_count != 2 ||
      frame_state(initial_throw_started.handle) != 0 ||
      !frame_done(initial_throw_started.handle)) {
    return 221;
  }
  destroy_coroutine(initial_throw_started.handle);

  int yield_exception_before = coroutine_unhandled_exception_count;
  YieldExceptionTask yield_value_throw_started = coroutine_yield_value_throws();
  if (yield_value_throw_started.value != 0 ||
      yield_value_throw_started.handle == 0 ||
      frame_state(yield_value_throw_started.handle) != 0 ||
      frame_done(yield_value_throw_started.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_value_throw_started.handle);
  if (last_resume_value != -929 ||
      coroutine_unhandled_exception_count != yield_exception_before + 1 ||
      frame_state(yield_value_throw_started.handle) != 0 ||
      !frame_done(yield_value_throw_started.handle)) {
    return 254;
  }
  destroy_coroutine(yield_value_throw_started.handle);

  int yield_throw_ctor_before = throwing_awaiter_ctor_count;
  int yield_throw_copy_before = throwing_awaiter_copy_count;
  int yield_throw_dtor_before = throwing_awaiter_dtor_count;
  YieldExceptionTask yield_await_resume_throw_started =
      coroutine_yield_await_resume_throw_cleans_awaiter();
  if (yield_await_resume_throw_started.value != 0 ||
      yield_await_resume_throw_started.handle == 0 ||
      frame_state(yield_await_resume_throw_started.handle) != 0 ||
      frame_done(yield_await_resume_throw_started.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_await_resume_throw_started.handle);
  if (last_resume_value != 882 ||
      throwing_awaiter_ctor_count != yield_throw_ctor_before + 1 ||
      throwing_awaiter_copy_count != yield_throw_copy_before + 1 ||
      frame_state(yield_await_resume_throw_started.handle) != 1 ||
      frame_done(yield_await_resume_throw_started.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_await_resume_throw_started.handle);
  if (last_resume_value != -929 ||
      coroutine_unhandled_exception_count != yield_exception_before + 2 ||
      throwing_awaiter_dtor_count != yield_throw_dtor_before + 1 ||
      frame_state(yield_await_resume_throw_started.handle) != 0 ||
      !frame_done(yield_await_resume_throw_started.handle)) {
    return 254;
  }
  destroy_coroutine(yield_await_resume_throw_started.handle);
  if (throwing_awaiter_dtor_count != yield_throw_dtor_before + 1) {
    return 254;
  }

  int final_always_suspend_before = final_always_suspend_count;
  int final_always_dtor_before = final_always_dtor_count;
  last_frame_handle = 0;
  FinalSuspendTask finally_suspended = coroutine_finally_suspended();
  if (finally_suspended.value != 515) {
    return 165;
  }
  if (final_always_suspend_count != final_always_suspend_before + 1) {
    return 171;
  }
  if (final_always_resume_count != 0) {
    return 172;
  }
  if (final_always_dtor_count != final_always_dtor_before) {
    return 176;
  }
  if (finally_suspended.handle == 0) {
    return 168;
  }
  if (finally_suspended.handle != final_always_suspend_handle) {
    return 169;
  }
  if (finally_suspended.handle != last_frame_handle) {
    return 170;
  }
  if (frame_state(finally_suspended.handle) != 0 ||
      !frame_done(finally_suspended.handle)) {
    return 173;
  }
  CoroutineFrame* finally_suspended_frame =
      (CoroutineFrame*)finally_suspended.handle;
  if (finally_suspended_frame->resume != 0 ||
      finally_suspended_frame->destroy == 0) {
    return 166;
  }
  resume_coroutine(finally_suspended.handle);
  if (last_resume_value != -1 ||
      final_always_suspend_count != final_always_suspend_before + 1 ||
      final_always_resume_count != 0 ||
      final_always_dtor_count != final_always_dtor_before) {
    return 167;
  }
  destroy_coroutine(finally_suspended.handle);
  if (final_always_dtor_count != final_always_dtor_before + 1) {
    return 177;
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
      persisted_local_ctor_count != 2 ||
      persisted_local_copy_count != 0 ||
      frame_state(persist_class_local_started.handle) != 1) {
    return 60;
  }
  int persisted_local_dtor_before_completion = persisted_local_dtor_count;
  resume_coroutine(persist_class_local_started.handle);
  if (last_resume_value != 46 ||
      persisted_local_dtor_count != persisted_local_dtor_before_completion + 1 ||
      frame_state(persist_class_local_started.handle) != 0 ||
      !frame_done(persist_class_local_started.handle)) {
    return 61;
  }
  int persisted_local_dtor_before_destroy = persisted_local_dtor_count;
  destroy_coroutine(persist_class_local_started.handle);
  if (persisted_local_dtor_count != persisted_local_dtor_before_destroy) {
    return 62;
  }

  Task persist_move_local_started =
      coroutine_persist_move_only_class_local_suspend();
  if (persist_move_local_started.value != 0 ||
      persist_move_local_started.handle == 0 ||
      persisted_move_local_ctor_count != 1 ||
      persisted_move_local_move_count != 0 ||
      frame_state(persist_move_local_started.handle) != 1) {
    return 190;
  }
  int persisted_move_local_dtor_before_completion =
      persisted_move_local_dtor_count;
  resume_coroutine(persist_move_local_started.handle);
  if (last_resume_value != 78 ||
      persisted_move_local_dtor_count !=
          persisted_move_local_dtor_before_completion + 1 ||
      frame_state(persist_move_local_started.handle) != 0 ||
      !frame_done(persist_move_local_started.handle)) {
    return 191;
  }
  int persisted_move_local_dtor_before_destroy =
      persisted_move_local_dtor_count;
  destroy_coroutine(persist_move_local_started.handle);
  if (persisted_move_local_dtor_count !=
      persisted_move_local_dtor_before_destroy) {
    return 192;
  }

  Task persist_immovable_local_started =
      coroutine_persist_immovable_class_local_suspend();
  if (persist_immovable_local_started.value != 0 ||
      persist_immovable_local_started.handle == 0 ||
      persisted_immovable_local_ctor_count != 1 ||
      frame_state(persist_immovable_local_started.handle) != 1) {
    return 254;
  }
  int persisted_immovable_dtor_before_completion =
      persisted_immovable_local_dtor_count;
  resume_coroutine(persist_immovable_local_started.handle);
  if (last_resume_value != 89 ||
      persisted_immovable_local_dtor_count !=
          persisted_immovable_dtor_before_completion + 1 ||
      frame_state(persist_immovable_local_started.handle) != 0 ||
      !frame_done(persist_immovable_local_started.handle)) {
    return 254;
  }
  int persisted_immovable_dtor_before_destroy =
      persisted_immovable_local_dtor_count;
  destroy_coroutine(persist_immovable_local_started.handle);
  if (persisted_immovable_local_dtor_count !=
      persisted_immovable_dtor_before_destroy) {
    return 254;
  }

  Task destroy_immovable_local_while_suspended =
      coroutine_persist_immovable_class_local_suspend();
  if (destroy_immovable_local_while_suspended.value != 0 ||
      destroy_immovable_local_while_suspended.handle == 0 ||
      persisted_immovable_local_ctor_count != 2 ||
      frame_state(destroy_immovable_local_while_suspended.handle) != 1) {
    return 254;
  }
  persisted_immovable_dtor_before_destroy =
      persisted_immovable_local_dtor_count;
  destroy_coroutine(destroy_immovable_local_while_suspended.handle);
  if (persisted_immovable_local_dtor_count !=
      persisted_immovable_dtor_before_destroy + 1) {
    return 254;
  }

  persisted_local_dtor_order = 0;
  Task nested_local_order_started =
      coroutine_nested_persisted_local_destruction_order();
  if (nested_local_order_started.value != 0 ||
      nested_local_order_started.handle == 0 ||
      frame_state(nested_local_order_started.handle) != 1) {
    return 254;
  }
  resume_coroutine(nested_local_order_started.handle);
  if (last_resume_value != 7 || persisted_local_dtor_order != 21 ||
      frame_state(nested_local_order_started.handle) != 0 ||
      !frame_done(nested_local_order_started.handle)) {
    return 254;
  }
  destroy_coroutine(nested_local_order_started.handle);
  if (persisted_local_dtor_order != 21) {
    return 254;
  }

  persisted_local_dtor_order = 0;
  Task destroy_nested_locals_while_suspended =
      coroutine_nested_persisted_local_destruction_order();
  if (destroy_nested_locals_while_suspended.value != 0 ||
      destroy_nested_locals_while_suspended.handle == 0 ||
      frame_state(destroy_nested_locals_while_suspended.handle) != 1) {
    return 254;
  }
  destroy_coroutine(destroy_nested_locals_while_suspended.handle);
  if (persisted_local_dtor_order != 21) {
    return 254;
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
      persisted_local_copy_count != 0) {
    return 73;
  }
  resume_coroutine(nested_branch_local_started.handle);
  if (last_resume_value != 11 ||
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

  Task yield_if_condition = coroutine_co_yield_if_condition();
  if (yield_if_condition.value != 911 || yield_if_condition.handle == 0 ||
      frame_state(yield_if_condition.handle) != 1 ||
      frame_done(yield_if_condition.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_if_condition.handle);
  if (last_resume_value != 913 ||
      frame_state(yield_if_condition.handle) != 0 ||
      !frame_done(yield_if_condition.handle)) {
    return 254;
  }
  destroy_coroutine(yield_if_condition.handle);

  Task yield_while_condition = coroutine_co_yield_while_condition();
  if (yield_while_condition.value != 1 ||
      yield_while_condition.handle == 0 ||
      frame_state(yield_while_condition.handle) != 1 ||
      frame_done(yield_while_condition.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_while_condition.handle);
  if (last_resume_value != 1 ||
      frame_state(yield_while_condition.handle) != 1 ||
      frame_done(yield_while_condition.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_while_condition.handle);
  if (last_resume_value != 0 ||
      frame_state(yield_while_condition.handle) != 1 ||
      frame_done(yield_while_condition.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_while_condition.handle);
  if (last_resume_value != 923 ||
      frame_state(yield_while_condition.handle) != 0 ||
      !frame_done(yield_while_condition.handle)) {
    return 254;
  }
  destroy_coroutine(yield_while_condition.handle);

  Task yield_do_condition = coroutine_co_yield_do_condition_continue();
  if (yield_do_condition.value != 1 || yield_do_condition.handle == 0 ||
      frame_state(yield_do_condition.handle) != 1 ||
      frame_done(yield_do_condition.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_do_condition.handle);
  if (last_resume_value != 0 || frame_state(yield_do_condition.handle) != 1 ||
      frame_done(yield_do_condition.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_do_condition.handle);
  if (last_resume_value != 942 ||
      frame_state(yield_do_condition.handle) != 0 ||
      !frame_done(yield_do_condition.handle)) {
    return 254;
  }
  destroy_coroutine(yield_do_condition.handle);

  Task yield_switch_condition = coroutine_co_yield_switch_condition();
  if (yield_switch_condition.value != 2 ||
      yield_switch_condition.handle == 0 ||
      frame_state(yield_switch_condition.handle) != 1 ||
      frame_done(yield_switch_condition.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_switch_condition.handle);
  if (last_resume_value != 942 ||
      frame_state(yield_switch_condition.handle) != 0 ||
      !frame_done(yield_switch_condition.handle)) {
    return 254;
  }
  destroy_coroutine(yield_switch_condition.handle);

  Task yield_short_circuit = coroutine_co_yield_short_circuit_condition();
  if (yield_short_circuit.value != 1 || yield_short_circuit.handle == 0 ||
      frame_state(yield_short_circuit.handle) != 1 ||
      frame_done(yield_short_circuit.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_short_circuit.handle);
  if (last_resume_value != 0 || frame_state(yield_short_circuit.handle) != 2 ||
      frame_done(yield_short_circuit.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_short_circuit.handle);
  if (last_resume_value != 950 ||
      frame_state(yield_short_circuit.handle) != 0 ||
      !frame_done(yield_short_circuit.handle)) {
    return 254;
  }
  destroy_coroutine(yield_short_circuit.handle);

  Task yield_conditional = coroutine_co_yield_conditional_expression();
  if (yield_conditional.value != 1 || yield_conditional.handle == 0 ||
      frame_state(yield_conditional.handle) != 1 ||
      frame_done(yield_conditional.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_conditional.handle);
  if (last_resume_value != 5 || frame_state(yield_conditional.handle) != 2 ||
      frame_done(yield_conditional.handle)) {
    return 254;
  }
  resume_yield_coroutine(yield_conditional.handle);
  if (last_resume_value != 965 || frame_state(yield_conditional.handle) != 0 ||
      !frame_done(yield_conditional.handle)) {
    return 254;
  }
  destroy_coroutine(yield_conditional.handle);

  Task mixed_logand = coroutine_mixed_co_await_co_yield_logand();
  if (mixed_logand.value != 0 || mixed_logand.handle == 0 ||
      frame_state(mixed_logand.handle) != 1 || frame_done(mixed_logand.handle)) {
    return 254;
  }
  resume_yield_coroutine(mixed_logand.handle);
  if (last_resume_value != 1 || frame_state(mixed_logand.handle) != 2 ||
      frame_done(mixed_logand.handle)) {
    return 254;
  }
  resume_yield_coroutine(mixed_logand.handle);
  if (last_resume_value != 971 || frame_state(mixed_logand.handle) != 0 ||
      !frame_done(mixed_logand.handle) || short_circuit_left_counter != 1) {
    return 254;
  }
  destroy_coroutine(mixed_logand.handle);

  Task mixed_logor_skip = coroutine_mixed_co_await_co_yield_logor_skip();
  if (mixed_logor_skip.value != 0 || mixed_logor_skip.handle == 0 ||
      frame_state(mixed_logor_skip.handle) != 1 ||
      frame_done(mixed_logor_skip.handle)) {
    return 254;
  }
  resume_yield_coroutine(mixed_logor_skip.handle);
  if (last_resume_value != 991 || frame_state(mixed_logor_skip.handle) != 0 ||
      !frame_done(mixed_logor_skip.handle) ||
      short_circuit_left_counter != 1 || short_circuit_right_counter != 0) {
    return 254;
  }
  destroy_coroutine(mixed_logor_skip.handle);

  Task mixed_conditional = coroutine_mixed_co_await_co_yield_conditional();
  if (mixed_conditional.value != 0 || mixed_conditional.handle == 0 ||
      frame_state(mixed_conditional.handle) != 1 ||
      frame_done(mixed_conditional.handle)) {
    return 254;
  }
  resume_yield_coroutine(mixed_conditional.handle);
  if (last_resume_value != 7 || frame_state(mixed_conditional.handle) != 3 ||
      frame_done(mixed_conditional.handle)) {
    return 254;
  }
  resume_yield_coroutine(mixed_conditional.handle);
  if (last_resume_value != 997 || frame_state(mixed_conditional.handle) != 0 ||
      !frame_done(mixed_conditional.handle) ||
      short_circuit_left_counter != 1) {
    return 254;
  }
  destroy_coroutine(mixed_conditional.handle);

  Task mixed_comma = coroutine_mixed_co_yield_co_await_comma();
  if (mixed_comma.value != 4 || mixed_comma.handle == 0 ||
      frame_state(mixed_comma.handle) != 1 || frame_done(mixed_comma.handle)) {
    return 254;
  }
  resume_yield_coroutine(mixed_comma.handle);
  if (last_resume_value != 4 || frame_state(mixed_comma.handle) != 2 ||
      frame_done(mixed_comma.handle)) {
    return 254;
  }
  resume_yield_coroutine(mixed_comma.handle);
  if (last_resume_value != 1006 || frame_state(mixed_comma.handle) != 0 ||
      !frame_done(mixed_comma.handle) || short_circuit_right_counter != 1) {
    return 254;
  }
  destroy_coroutine(mixed_comma.handle);

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

  Task statement_await_named_started = coroutine_co_await_statement_named();
  if (statement_await_named_started.value != 0 ||
      statement_await_named_started.handle == 0 ||
      frame_state(statement_await_named_started.handle) != 1) {
    return 174;
  }
  resume_coroutine(statement_await_named_started.handle);
  if (last_resume_value != 17 ||
      frame_state(statement_await_named_started.handle) != 0 ||
      !frame_done(statement_await_named_started.handle)) {
    return 175;
  }
  destroy_coroutine(statement_await_named_started.handle);

  Task statement_await_temporary_started =
      coroutine_co_await_statement_temporary();
  if (statement_await_temporary_started.value != 0 ||
      statement_await_temporary_started.handle == 0 ||
      frame_state(statement_await_temporary_started.handle) != 1) {
    return 176;
  }
  resume_coroutine(statement_await_temporary_started.handle);
  if (last_resume_value != 19 ||
      frame_state(statement_await_temporary_started.handle) != 0 ||
      !frame_done(statement_await_temporary_started.handle)) {
    return 177;
  }
  destroy_coroutine(statement_await_temporary_started.handle);

  Task assignment_await_started = coroutine_co_await_assignment_rhs();
  if (assignment_await_started.value != 0 ||
      assignment_await_started.handle == 0 ||
      frame_state(assignment_await_started.handle) != 1) {
    return 178;
  }
  resume_coroutine(assignment_await_started.handle);
  if (last_resume_value != 194 ||
      frame_state(assignment_await_started.handle) != 0 ||
      !frame_done(assignment_await_started.handle)) {
    return 179;
  }
  destroy_coroutine(assignment_await_started.handle);

  SuspendValue parameter_awaiter = {241};
  Task parameter_await_started = coroutine_co_await_parameter(parameter_awaiter);
  if (parameter_await_started.value != 0 ||
      parameter_await_started.handle == 0 ||
      frame_state(parameter_await_started.handle) != 1) {
    return 221;
  }
  resume_coroutine(parameter_await_started.handle);
  if (last_resume_value != 246 ||
      frame_state(parameter_await_started.handle) != 0 ||
      !frame_done(parameter_await_started.handle)) {
    return 222;
  }
  destroy_coroutine(parameter_await_started.handle);

  int move_only_moves_before = move_only_awaiter_move_count;
  Task move_only_named_started = coroutine_move_only_named_awaiter();
  if (move_only_named_started.value != 0 ||
      move_only_named_started.handle == 0 ||
      move_only_awaiter_move_count != move_only_moves_before + 1 ||
      frame_state(move_only_named_started.handle) != 1) {
    return 223;
  }
  resume_coroutine(move_only_named_started.handle);
  if (last_resume_value != 656 ||
      frame_state(move_only_named_started.handle) != 0 ||
      !frame_done(move_only_named_started.handle)) {
    return 224;
  }
  int move_only_dtor_before_destroy = move_only_awaiter_dtor_count;
  destroy_coroutine(move_only_named_started.handle);
  if (move_only_awaiter_dtor_count != move_only_dtor_before_destroy) {
    return 225;
  }

  Task coreturn_await_started = coroutine_co_return_co_await();
  if (coreturn_await_started.value != 0 ||
      coreturn_await_started.handle == 0 ||
      frame_state(coreturn_await_started.handle) != 1) {
    return 180;
  }
  resume_coroutine(coreturn_await_started.handle);
  if (last_resume_value != 197 ||
      frame_state(coreturn_await_started.handle) != 0 ||
      !frame_done(coreturn_await_started.handle)) {
    return 181;
  }
  destroy_coroutine(coreturn_await_started.handle);

  Task binary_initializer_started = coroutine_co_await_binary_initializer();
  if (binary_initializer_started.value != 0 ||
      binary_initializer_started.handle == 0 ||
      frame_state(binary_initializer_started.handle) != 1) {
    return 182;
  }
  resume_coroutine(binary_initializer_started.handle);
  if (last_resume_value != 211 ||
      frame_state(binary_initializer_started.handle) != 0 ||
      !frame_done(binary_initializer_started.handle)) {
    return 183;
  }
  destroy_coroutine(binary_initializer_started.handle);

  last_resume_value = 0;
  Task multi_binary_initializer_started =
      coroutine_co_await_multiple_binary_initializer();
  if (multi_binary_initializer_started.value != 0 ||
      multi_binary_initializer_started.handle == 0 ||
      frame_state(multi_binary_initializer_started.handle) != 1) {
    return 251;
  }
  resume_coroutine(multi_binary_initializer_started.handle);
  if (last_resume_value != 0 ||
      frame_state(multi_binary_initializer_started.handle) != 2 ||
      frame_done(multi_binary_initializer_started.handle)) {
    return 252;
  }
  resume_coroutine(multi_binary_initializer_started.handle);
  if (last_resume_value != 464 ||
      frame_state(multi_binary_initializer_started.handle) != 0 ||
      !frame_done(multi_binary_initializer_started.handle)) {
    return 253;
  }
  destroy_coroutine(multi_binary_initializer_started.handle);

  last_resume_value = 0;
  Task logand_skipped_started = coroutine_co_await_logand_right_skipped();
  if (logand_skipped_started.value != 0 ||
      logand_skipped_started.handle == 0 ||
      frame_state(logand_skipped_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 151;
  }
  resume_coroutine(logand_skipped_started.handle);
  if (last_resume_value != 700 ||
      frame_state(logand_skipped_started.handle) != 0 ||
      !frame_done(logand_skipped_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 0) {
    return 152;
  }
  destroy_coroutine(logand_skipped_started.handle);

  last_resume_value = 0;
  Task logand_taken_started = coroutine_co_await_logand_right_taken();
  if (logand_taken_started.value != 0 ||
      logand_taken_started.handle == 0 ||
      frame_state(logand_taken_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 153;
  }
  resume_coroutine(logand_taken_started.handle);
  if (last_resume_value != 0 ||
      frame_state(logand_taken_started.handle) != 2 ||
      frame_done(logand_taken_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 0) {
    return 154;
  }
  resume_coroutine(logand_taken_started.handle);
  if (last_resume_value != 711 ||
      frame_state(logand_taken_started.handle) != 0 ||
      !frame_done(logand_taken_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 1) {
    return 155;
  }
  destroy_coroutine(logand_taken_started.handle);

  last_resume_value = 0;
  Task logor_skipped_started = coroutine_co_await_logor_right_skipped();
  if (logor_skipped_started.value != 0 ||
      logor_skipped_started.handle == 0 ||
      frame_state(logor_skipped_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 156;
  }
  resume_coroutine(logor_skipped_started.handle);
  if (last_resume_value != 721 ||
      frame_state(logor_skipped_started.handle) != 0 ||
      !frame_done(logor_skipped_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 0) {
    return 157;
  }
  destroy_coroutine(logor_skipped_started.handle);

  last_resume_value = 0;
  Task logor_taken_started = coroutine_co_await_logor_right_taken();
  if (logor_taken_started.value != 0 ||
      logor_taken_started.handle == 0 ||
      frame_state(logor_taken_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 158;
  }
  resume_coroutine(logor_taken_started.handle);
  if (last_resume_value != 0 ||
      frame_state(logor_taken_started.handle) != 2 ||
      frame_done(logor_taken_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 0) {
    return 159;
  }
  resume_coroutine(logor_taken_started.handle);
  if (last_resume_value != 731 ||
      frame_state(logor_taken_started.handle) != 0 ||
      !frame_done(logor_taken_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 1) {
    return 160;
  }
  destroy_coroutine(logor_taken_started.handle);

  last_resume_value = 0;
  Task conditional_true_started =
      coroutine_co_await_conditional_true_arm();
  if (conditional_true_started.value != 0 ||
      conditional_true_started.handle == 0 ||
      frame_state(conditional_true_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_true_counter != 0 ||
      short_circuit_false_counter != 0) {
    return 161;
  }
  resume_coroutine(conditional_true_started.handle);
  if (last_resume_value != 0 ||
      frame_state(conditional_true_started.handle) != 2 ||
      frame_done(conditional_true_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_true_counter != 0 ||
      short_circuit_false_counter != 0) {
    return 162;
  }
  resume_coroutine(conditional_true_started.handle);
  if (last_resume_value != 317 ||
      frame_state(conditional_true_started.handle) != 0 ||
      !frame_done(conditional_true_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_true_counter != 1 ||
      short_circuit_false_counter != 0) {
    return 163;
  }
  destroy_coroutine(conditional_true_started.handle);

  last_resume_value = 0;
  Task conditional_false_started =
      coroutine_co_await_conditional_false_arm();
  if (conditional_false_started.value != 0 ||
      conditional_false_started.handle == 0 ||
      frame_state(conditional_false_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_true_counter != 0 ||
      short_circuit_false_counter != 0) {
    return 164;
  }
  resume_coroutine(conditional_false_started.handle);
  if (last_resume_value != 0 ||
      frame_state(conditional_false_started.handle) != 3 ||
      frame_done(conditional_false_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_true_counter != 0 ||
      short_circuit_false_counter != 0) {
    return 165;
  }
  resume_coroutine(conditional_false_started.handle);
  if (last_resume_value != 419 ||
      frame_state(conditional_false_started.handle) != 0 ||
      !frame_done(conditional_false_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_true_counter != 0 ||
      short_circuit_false_counter != 1) {
    return 166;
  }
  destroy_coroutine(conditional_false_started.handle);

  last_resume_value = 0;
  Task comma_sequence_started = coroutine_co_await_comma_sequence();
  if (comma_sequence_started.value != 0 ||
      comma_sequence_started.handle == 0 ||
      frame_state(comma_sequence_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 167;
  }
  resume_coroutine(comma_sequence_started.handle);
  if (last_resume_value != 0 ||
      frame_state(comma_sequence_started.handle) != 2 ||
      frame_done(comma_sequence_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 0) {
    return 168;
  }
  resume_coroutine(comma_sequence_started.handle);
  if (last_resume_value != 418 ||
      frame_state(comma_sequence_started.handle) != 0 ||
      !frame_done(comma_sequence_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 1) {
    return 169;
  }
  destroy_coroutine(comma_sequence_started.handle);

  Task binary_assignment_started = coroutine_co_await_binary_assignment_rhs();
  if (binary_assignment_started.value != 0 ||
      binary_assignment_started.handle == 0 ||
      frame_state(binary_assignment_started.handle) != 1) {
    return 184;
  }
  resume_coroutine(binary_assignment_started.handle);
  if (last_resume_value != 224 ||
      frame_state(binary_assignment_started.handle) != 0 ||
      !frame_done(binary_assignment_started.handle)) {
    return 185;
  }
  destroy_coroutine(binary_assignment_started.handle);

  Task call_argument_started = coroutine_co_await_call_argument();
  if (call_argument_started.value != 0 ||
      call_argument_started.handle == 0 ||
      frame_state(call_argument_started.handle) != 1) {
    return 186;
  }
  resume_coroutine(call_argument_started.handle);
  if (last_resume_value != 241 ||
      frame_state(call_argument_started.handle) != 0 ||
      !frame_done(call_argument_started.handle)) {
    return 187;
  }
  destroy_coroutine(call_argument_started.handle);

  Task if_condition_started = coroutine_co_await_if_condition();
  if (if_condition_started.value != 0 ||
      if_condition_started.handle == 0 ||
      frame_state(if_condition_started.handle) != 1) {
    return 233;
  }
  resume_coroutine(if_condition_started.handle);
  if (last_resume_value != 273 ||
      frame_state(if_condition_started.handle) != 0 ||
      !frame_done(if_condition_started.handle)) {
    return 234;
  }
  destroy_coroutine(if_condition_started.handle);

  Task while_condition_started = coroutine_co_await_while_condition();
  if (while_condition_started.value != 0 ||
      while_condition_started.handle == 0 ||
      frame_state(while_condition_started.handle) != 1 ||
      condition_while_counter != 0) {
    return 235;
  }
  resume_coroutine(while_condition_started.handle);
  if (last_resume_value != 0 ||
      frame_state(while_condition_started.handle) != 1 ||
      frame_done(while_condition_started.handle) ||
      condition_while_counter != 1) {
    return 236;
  }
  resume_coroutine(while_condition_started.handle);
  if (last_resume_value != 0 ||
      frame_state(while_condition_started.handle) != 1 ||
      frame_done(while_condition_started.handle) ||
      condition_while_counter != 2) {
    return 237;
  }
  resume_coroutine(while_condition_started.handle);
  if (last_resume_value != 303 ||
      frame_state(while_condition_started.handle) != 0 ||
      !frame_done(while_condition_started.handle) ||
      condition_while_counter != 3) {
    return 238;
  }
  destroy_coroutine(while_condition_started.handle);

  Task for_condition_started = coroutine_co_await_for_condition();
  if (for_condition_started.value != 0 ||
      for_condition_started.handle == 0 ||
      frame_state(for_condition_started.handle) != 1 ||
      condition_for_counter != 0) {
    return 239;
  }
  resume_coroutine(for_condition_started.handle);
  if (last_resume_value != 0 ||
      frame_state(for_condition_started.handle) != 1 ||
      frame_done(for_condition_started.handle) ||
      condition_for_counter != 1) {
    return 240;
  }
  resume_coroutine(for_condition_started.handle);
  if (last_resume_value != 0 ||
      frame_state(for_condition_started.handle) != 1 ||
      frame_done(for_condition_started.handle) ||
      condition_for_counter != 2) {
    return 241;
  }
  resume_coroutine(for_condition_started.handle);
  if (last_resume_value != 406 ||
      frame_state(for_condition_started.handle) != 0 ||
      !frame_done(for_condition_started.handle) ||
      condition_for_counter != 3) {
    return 242;
  }
  destroy_coroutine(for_condition_started.handle);

  Task do_condition_started = coroutine_co_await_do_condition();
  if (do_condition_started.value != 0 ||
      do_condition_started.handle == 0 ||
      frame_state(do_condition_started.handle) != 1 ||
      condition_do_counter != 0) {
    return 243;
  }
  resume_coroutine(do_condition_started.handle);
  if (last_resume_value != 0 ||
      frame_state(do_condition_started.handle) != 1 ||
      frame_done(do_condition_started.handle) ||
      condition_do_counter != 1) {
    return 244;
  }
  resume_coroutine(do_condition_started.handle);
  if (last_resume_value != 0 ||
      frame_state(do_condition_started.handle) != 1 ||
      frame_done(do_condition_started.handle) ||
      condition_do_counter != 2) {
    return 245;
  }
  resume_coroutine(do_condition_started.handle);
  if (last_resume_value != 503 ||
      frame_state(do_condition_started.handle) != 0 ||
      !frame_done(do_condition_started.handle) ||
      condition_do_counter != 3) {
    return 246;
  }
  destroy_coroutine(do_condition_started.handle);

  Task do_continue_started = coroutine_co_await_do_condition_continue();
  if (do_continue_started.value != 0 ||
      do_continue_started.handle == 0 ||
      frame_state(do_continue_started.handle) != 1 ||
      condition_do_continue_counter != 0) {
    return 247;
  }
  resume_coroutine(do_continue_started.handle);
  if (last_resume_value != 0 ||
      frame_state(do_continue_started.handle) != 1 ||
      frame_done(do_continue_started.handle) ||
      condition_do_continue_counter != 1) {
    return 248;
  }
  resume_coroutine(do_continue_started.handle);
  if (last_resume_value != 0 ||
      frame_state(do_continue_started.handle) != 1 ||
      frame_done(do_continue_started.handle) ||
      condition_do_continue_counter != 2) {
    return 249;
  }
  resume_coroutine(do_continue_started.handle);
  if (last_resume_value != 730 ||
      frame_state(do_continue_started.handle) != 0 ||
      !frame_done(do_continue_started.handle) ||
      condition_do_continue_counter != 3) {
    return 250;
  }
  destroy_coroutine(do_continue_started.handle);

  last_resume_value = 0;
  Task while_short_circuit_started =
      coroutine_co_await_while_short_circuit_condition();
  if (while_short_circuit_started.value != 0 ||
      while_short_circuit_started.handle == 0 ||
      frame_state(while_short_circuit_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 254;
  }
  resume_coroutine(while_short_circuit_started.handle);
  resume_coroutine(while_short_circuit_started.handle);
  resume_coroutine(while_short_circuit_started.handle);
  resume_coroutine(while_short_circuit_started.handle);
  if (frame_done(while_short_circuit_started.handle)) {
    return 254;
  }
  resume_coroutine(while_short_circuit_started.handle);
  if (last_resume_value != 1153 ||
      frame_state(while_short_circuit_started.handle) != 0 ||
      !frame_done(while_short_circuit_started.handle) ||
      short_circuit_left_counter != 3 ||
      short_circuit_right_counter != 2) {
    return 254;
  }
  destroy_coroutine(while_short_circuit_started.handle);

  last_resume_value = 0;
  Task for_short_circuit_started =
      coroutine_co_await_for_short_circuit_condition();
  if (for_short_circuit_started.value != 0 ||
      for_short_circuit_started.handle == 0 ||
      frame_state(for_short_circuit_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 254;
  }
  resume_coroutine(for_short_circuit_started.handle);
  resume_coroutine(for_short_circuit_started.handle);
  resume_coroutine(for_short_circuit_started.handle);
  resume_coroutine(for_short_circuit_started.handle);
  resume_coroutine(for_short_circuit_started.handle);
  if (frame_done(for_short_circuit_started.handle)) {
    return 254;
  }
  resume_coroutine(for_short_circuit_started.handle);
  if (last_resume_value != 1242 ||
      frame_state(for_short_circuit_started.handle) != 0 ||
      !frame_done(for_short_circuit_started.handle) ||
      short_circuit_left_counter != 4 ||
      short_circuit_right_counter != 2) {
    return 254;
  }
  destroy_coroutine(for_short_circuit_started.handle);

  last_resume_value = 0;
  Task do_short_circuit_continue_started =
      coroutine_co_await_do_short_circuit_continue_condition();
  if (do_short_circuit_continue_started.value != 0 ||
      do_short_circuit_continue_started.handle == 0 ||
      frame_state(do_short_circuit_continue_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 254;
  }
  resume_coroutine(do_short_circuit_continue_started.handle);
  resume_coroutine(do_short_circuit_continue_started.handle);
  resume_coroutine(do_short_circuit_continue_started.handle);
  resume_coroutine(do_short_circuit_continue_started.handle);
  if (frame_done(do_short_circuit_continue_started.handle)) {
    return 254;
  }
  resume_coroutine(do_short_circuit_continue_started.handle);
  if (last_resume_value != 1562 ||
      frame_state(do_short_circuit_continue_started.handle) != 0 ||
      !frame_done(do_short_circuit_continue_started.handle) ||
      short_circuit_left_counter != 3 ||
      short_circuit_right_counter != 2) {
    return 254;
  }
  destroy_coroutine(do_short_circuit_continue_started.handle);

  last_resume_value = 0;
  Task switch_condition_started = coroutine_co_await_switch_condition();
  if (switch_condition_started.value != 0 ||
      switch_condition_started.handle == 0 ||
      frame_state(switch_condition_started.handle) != 1 ||
      short_circuit_left_counter != 0) {
    return 254;
  }
  resume_coroutine(switch_condition_started.handle);
  if (last_resume_value != 1012 ||
      frame_state(switch_condition_started.handle) != 0 ||
      !frame_done(switch_condition_started.handle) ||
      short_circuit_left_counter != 1) {
    return 254;
  }
  destroy_coroutine(switch_condition_started.handle);

  last_resume_value = 0;
  Task switch_binary_started = coroutine_co_await_switch_binary_condition();
  if (switch_binary_started.value != 0 ||
      switch_binary_started.handle == 0 ||
      frame_state(switch_binary_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 254;
  }
  resume_coroutine(switch_binary_started.handle);
  if (last_resume_value != 0 ||
      frame_state(switch_binary_started.handle) != 2 ||
      frame_done(switch_binary_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 0) {
    return 254;
  }
  resume_coroutine(switch_binary_started.handle);
  if (last_resume_value != 1118 ||
      frame_state(switch_binary_started.handle) != 0 ||
      !frame_done(switch_binary_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 1) {
    return 254;
  }
  destroy_coroutine(switch_binary_started.handle);

  last_resume_value = 0;
  Task switch_conditional_started =
      coroutine_co_await_switch_conditional_condition();
  if (switch_conditional_started.value != 0 ||
      switch_conditional_started.handle == 0 ||
      frame_state(switch_conditional_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_true_counter != 0 ||
      short_circuit_false_counter != 0) {
    return 254;
  }
  resume_coroutine(switch_conditional_started.handle);
  if (last_resume_value != 0 ||
      frame_state(switch_conditional_started.handle) != 3 ||
      frame_done(switch_conditional_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_true_counter != 0 ||
      short_circuit_false_counter != 0) {
    return 254;
  }
  resume_coroutine(switch_conditional_started.handle);
  if (last_resume_value != 1305 ||
      frame_state(switch_conditional_started.handle) != 0 ||
      !frame_done(switch_conditional_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_true_counter != 0 ||
      short_circuit_false_counter != 1) {
    return 254;
  }
  destroy_coroutine(switch_conditional_started.handle);

  last_resume_value = 0;
  Task nested_try_label_started = coroutine_co_await_nested_try_and_label();
  if (nested_try_label_started.value != 0 ||
      nested_try_label_started.handle == 0 ||
      frame_state(nested_try_label_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0 ||
      short_circuit_true_counter != 0) {
    return 254;
  }
  resume_coroutine(nested_try_label_started.handle);
  if (last_resume_value != 0 ||
      frame_state(nested_try_label_started.handle) != 2 ||
      frame_done(nested_try_label_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 0 ||
      short_circuit_true_counter != 0) {
    return 254;
  }
  resume_coroutine(nested_try_label_started.handle);
  if (last_resume_value != 0 ||
      frame_state(nested_try_label_started.handle) != 3 ||
      frame_done(nested_try_label_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 1 ||
      short_circuit_true_counter != 0) {
    return 254;
  }
  resume_coroutine(nested_try_label_started.handle);
  if (last_resume_value != 1416 ||
      frame_state(nested_try_label_started.handle) != 0 ||
      !frame_done(nested_try_label_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 1 ||
      short_circuit_true_counter != 1) {
    return 254;
  }
  destroy_coroutine(nested_try_label_started.handle);

  last_resume_value = 0;
  Task nested_case_started = coroutine_co_await_nested_switch_case_body();
  if (nested_case_started.value != 0 ||
      nested_case_started.handle == 0 ||
      frame_state(nested_case_started.handle) != 1 ||
      short_circuit_left_counter != 0) {
    return 254;
  }
  resume_coroutine(nested_case_started.handle);
  if (last_resume_value != 1414 ||
      frame_state(nested_case_started.handle) != 0 ||
      !frame_done(nested_case_started.handle) ||
      short_circuit_left_counter != 1) {
    return 254;
  }
  destroy_coroutine(nested_case_started.handle);

  last_resume_value = 0;
  Task nested_catch_started = coroutine_co_await_nested_catch_body();
  if (nested_catch_started.value != 0 ||
      nested_catch_started.handle == 0 ||
      frame_state(nested_catch_started.handle) != 1 ||
      short_circuit_left_counter != 0) {
    return 254;
  }
  resume_coroutine(nested_catch_started.handle);
  if (last_resume_value != 1534) {
    return 254;
  }
  if (frame_state(nested_catch_started.handle) != 0) {
    return 254;
  }
  if (!frame_done(nested_catch_started.handle)) {
    return 254;
  }
  if (short_circuit_left_counter != 1) {
    return 254;
  }
  destroy_coroutine(nested_catch_started.handle);

  last_resume_value = 0;
  Task catch_multi_started =
      coroutine_co_await_catch_parameter_multiple_suspensions();
  if (catch_multi_started.value != 0 ||
      catch_multi_started.handle == 0 ||
      frame_state(catch_multi_started.handle) != 1 ||
      short_circuit_left_counter != 0 ||
      short_circuit_right_counter != 0) {
    return 254;
  }
  resume_coroutine(catch_multi_started.handle);
  if (last_resume_value != 0 ||
      frame_state(catch_multi_started.handle) != 2 ||
      frame_done(catch_multi_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 0) {
    return 254;
  }
  resume_coroutine(catch_multi_started.handle);
  if (last_resume_value != 1759 ||
      frame_state(catch_multi_started.handle) != 0 ||
      !frame_done(catch_multi_started.handle) ||
      short_circuit_left_counter != 1 ||
      short_circuit_right_counter != 1) {
    return 254;
  }
  destroy_coroutine(catch_multi_started.handle);

  last_resume_value = 0;
  Task catch_nested_control_started =
      coroutine_co_await_nested_catch_parameter_control_flow();
  if (catch_nested_control_started.value != 0 ||
      catch_nested_control_started.handle == 0 ||
      frame_state(catch_nested_control_started.handle) != 1 ||
      short_circuit_left_counter != 0) {
    return 254;
  }
  resume_coroutine(catch_nested_control_started.handle);
  if (last_resume_value != 1736 ||
      frame_state(catch_nested_control_started.handle) != 0 ||
      !frame_done(catch_nested_control_started.handle) ||
      short_circuit_left_counter != 1) {
    return 254;
  }
  destroy_coroutine(catch_nested_control_started.handle);

  Task coreturn_binary_started = coroutine_co_return_binary_co_await();
  if (coreturn_binary_started.value != 0 ||
      coreturn_binary_started.handle == 0 ||
      frame_state(coreturn_binary_started.handle) != 1) {
    return 188;
  }
  resume_coroutine(coreturn_binary_started.handle);
  if (last_resume_value != 244 ||
      frame_state(coreturn_binary_started.handle) != 0 ||
      !frame_done(coreturn_binary_started.handle)) {
    return 189;
  }
  destroy_coroutine(coreturn_binary_started.handle);

  last_resume_value = 0;
  Task coreturn_multi_binary_started =
      coroutine_co_return_multiple_binary_co_await();
  if (coreturn_multi_binary_started.value != 0 ||
      coreturn_multi_binary_started.handle == 0 ||
      frame_state(coreturn_multi_binary_started.handle) != 1) {
    return 251;
  }
  resume_coroutine(coreturn_multi_binary_started.handle);
  if (last_resume_value != 0 ||
      frame_state(coreturn_multi_binary_started.handle) != 2 ||
      frame_done(coreturn_multi_binary_started.handle)) {
    return 252;
  }
  resume_coroutine(coreturn_multi_binary_started.handle);
  if (last_resume_value != 490 ||
      frame_state(coreturn_multi_binary_started.handle) != 0 ||
      !frame_done(coreturn_multi_binary_started.handle)) {
    return 253;
  }
  destroy_coroutine(coreturn_multi_binary_started.handle);

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

  int noncopy_ctor_before = noncopy_direct_awaiter_ctor_count;
  int noncopy_dtor_before = noncopy_direct_awaiter_dtor_count;
  Task direct_await_noncopy_started =
      coroutine_direct_await_noncopyable_expression();
  if (noncopy_direct_awaiter_ctor_count != noncopy_ctor_before + 1 ||
      noncopy_direct_awaiter_dtor_count != noncopy_dtor_before ||
      direct_await_noncopy_started.value != 0 ||
      direct_await_noncopy_started.handle == 0 ||
      frame_state(direct_await_noncopy_started.handle) != 1) {
    return 165;
  }
  resume_coroutine(direct_await_noncopy_started.handle);
  if (last_resume_value != 33 ||
      frame_state(direct_await_noncopy_started.handle) != 0 ||
      !frame_done(direct_await_noncopy_started.handle) ||
      noncopy_direct_awaiter_dtor_count != noncopy_dtor_before + 1) {
    return 166;
  }
  destroy_coroutine(direct_await_noncopy_started.handle);
  if (noncopy_direct_awaiter_dtor_count != noncopy_dtor_before + 1) {
    return 168;
  }

  noncopy_ctor_before = noncopy_direct_awaiter_ctor_count;
  noncopy_dtor_before = noncopy_direct_awaiter_dtor_count;
  Task direct_await_noncopy_loop_started =
      coroutine_direct_await_noncopyable_loop_expression();
  if (noncopy_direct_awaiter_ctor_count != noncopy_ctor_before + 1 ||
      noncopy_direct_awaiter_dtor_count != noncopy_dtor_before ||
      direct_await_noncopy_loop_started.value != 0 ||
      direct_await_noncopy_loop_started.handle == 0 ||
      frame_state(direct_await_noncopy_loop_started.handle) != 1) {
    return 169;
  }
  resume_coroutine(direct_await_noncopy_loop_started.handle);
  if (noncopy_direct_awaiter_ctor_count != noncopy_ctor_before + 2 ||
      noncopy_direct_awaiter_dtor_count != noncopy_dtor_before + 1 ||
      last_resume_value != 0 ||
      frame_state(direct_await_noncopy_loop_started.handle) != 1 ||
      frame_done(direct_await_noncopy_loop_started.handle)) {
    return 170;
  }
  resume_coroutine(direct_await_noncopy_loop_started.handle);
  if (last_resume_value != 101 ||
      frame_state(direct_await_noncopy_loop_started.handle) != 0 ||
      !frame_done(direct_await_noncopy_loop_started.handle) ||
      noncopy_direct_awaiter_dtor_count != noncopy_dtor_before + 2) {
    return 171;
  }
  destroy_coroutine(direct_await_noncopy_loop_started.handle);
  if (noncopy_direct_awaiter_dtor_count != noncopy_dtor_before + 2) {
    return 172;
  }

  noncopy_ctor_before = noncopy_direct_awaiter_ctor_count;
  noncopy_dtor_before = noncopy_direct_awaiter_dtor_count;
  Task direct_await_noncopy_return_started =
      coroutine_co_return_direct_noncopyable_co_await();
  if (noncopy_direct_awaiter_ctor_count != noncopy_ctor_before + 1 ||
      noncopy_direct_awaiter_dtor_count != noncopy_dtor_before ||
      direct_await_noncopy_return_started.value != 0 ||
      direct_await_noncopy_return_started.handle == 0 ||
      frame_state(direct_await_noncopy_return_started.handle) != 1) {
    return 173;
  }
  resume_coroutine(direct_await_noncopy_return_started.handle);
  if (last_resume_value != 63 ||
      frame_state(direct_await_noncopy_return_started.handle) != 0 ||
      !frame_done(direct_await_noncopy_return_started.handle) ||
      noncopy_direct_awaiter_dtor_count != noncopy_dtor_before + 1) {
    return 174;
  }
  destroy_coroutine(direct_await_noncopy_return_started.handle);
  if (noncopy_direct_awaiter_dtor_count != noncopy_dtor_before + 1) {
    return 175;
  }

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

  Task transfer_target_started = coroutine_transfer_target();
  if (transfer_target_started.value != 0 ||
      transfer_target_started.handle == 0 ||
      frame_state(transfer_target_started.handle) != 1 ||
      frame_done(transfer_target_started.handle)) {
    return 161;
  }
  transfer_target_handle = transfer_target_started.handle;
  int transfer_await_suspend_before = transfer_await_suspend_count;
  Task transfer_source_started = coroutine_transfer_source();
  if (transfer_source_started.value != 0 ||
      transfer_source_started.handle == 0 ||
      frame_state(transfer_source_started.handle) != 1 ||
      frame_done(transfer_source_started.handle) ||
      transfer_await_suspend_count != transfer_await_suspend_before + 1) {
    return 162;
  }
  if (frame_state(transfer_target_started.handle) != 0 ||
      !frame_done(transfer_target_started.handle) ||
      transfer_target_result != 216) {
    return 163;
  }
  resume_coroutine(transfer_source_started.handle);
  if (last_resume_value != 313 ||
      frame_state(transfer_source_started.handle) != 0 ||
      !frame_done(transfer_source_started.handle)) {
    return 164;
  }
  destroy_coroutine(transfer_target_started.handle);
  destroy_coroutine(transfer_source_started.handle);

  Task final_transfer_target_started = coroutine_transfer_target();
  if (final_transfer_target_started.value != 0 ||
      final_transfer_target_started.handle == 0 ||
      frame_state(final_transfer_target_started.handle) != 1 ||
      frame_done(final_transfer_target_started.handle)) {
    return 254;
  }
  transfer_target_handle = final_transfer_target_started.handle;
  transfer_await_suspend_before = transfer_await_suspend_count;
  FinalTransferTask final_transfer_source_started =
      coroutine_final_suspend_transfer_source();
  if (final_transfer_source_started.value != 123 ||
      final_transfer_source_started.handle == 0 ||
      frame_state(final_transfer_source_started.handle) != 0 ||
      !frame_done(final_transfer_source_started.handle) ||
      transfer_await_suspend_count != transfer_await_suspend_before + 1) {
    return 254;
  }
  if (frame_state(final_transfer_target_started.handle) != 0 ||
      !frame_done(final_transfer_target_started.handle) ||
      transfer_target_result != 216) {
    return 254;
  }
  destroy_coroutine(final_transfer_target_started.handle);
  destroy_coroutine(final_transfer_source_started.handle);

  Task handle_transfer_target_started = coroutine_transfer_target();
  if (handle_transfer_target_started.value != 0 ||
      handle_transfer_target_started.handle == 0 ||
      frame_state(handle_transfer_target_started.handle) != 1 ||
      frame_done(handle_transfer_target_started.handle)) {
    return 229;
  }
  transfer_target_handle = handle_transfer_target_started.handle;
  transfer_await_suspend_before = transfer_await_suspend_count;
  Task handle_transfer_source_started = coroutine_handle_transfer_source();
  if (handle_transfer_source_started.value != 0 ||
      handle_transfer_source_started.handle == 0 ||
      frame_state(handle_transfer_source_started.handle) != 1 ||
      frame_done(handle_transfer_source_started.handle) ||
      transfer_await_suspend_count != transfer_await_suspend_before + 1) {
    return 230;
  }
  if (frame_state(handle_transfer_target_started.handle) != 0 ||
      !frame_done(handle_transfer_target_started.handle) ||
      transfer_target_result != 216) {
    return 231;
  }
  resume_coroutine(handle_transfer_source_started.handle);
  if (last_resume_value != 323 ||
      frame_state(handle_transfer_source_started.handle) != 0 ||
      !frame_done(handle_transfer_source_started.handle)) {
    return 232;
  }
  destroy_coroutine(handle_transfer_target_started.handle);
  destroy_coroutine(handle_transfer_source_started.handle);

  TransformTask transformed_started = coroutine_await_transform_int();
  if (transformed_started.value != 0 ||
      transformed_started.handle == 0 ||
      await_transform_count != 1 ||
      frame_state(transformed_started.handle) != 1) {
    return 193;
  }
  resume_coroutine(transformed_started.handle);
  if (last_resume_value != 314 ||
      frame_state(transformed_started.handle) != 0 ||
      !frame_done(transformed_started.handle)) {
    return 194;
  }
  destroy_coroutine(transformed_started.handle);

  allocation_failure_next_new_fails = 1;
  AllocationFailureTask allocation_failed =
      coroutine_allocation_failure_value();
  if (allocation_failed.value != -777 ||
      allocation_failed.handle != 0 ||
      allocation_failure_operator_new_count != 1 ||
      allocation_failure_operator_delete_count != 0 ||
      allocation_failure_fallback_count != 1) {
    return 195;
  }

  AllocationFailureTask allocation_started =
      coroutine_allocation_failure_value();
  if (allocation_started.value != 0) {
    return 196;
  }
  if (allocation_started.handle == 0) {
    return 224;
  }
  if (allocation_failure_operator_new_count != 2) {
    return 225;
  }
  if (allocation_failure_fallback_count != 1) {
    return 226;
  }
  if (frame_state(allocation_started.handle) != 1) {
    return 227;
  }
  resume_coroutine(allocation_started.handle);
  if (last_resume_value != 701 ||
      frame_state(allocation_started.handle) != 0 ||
      !frame_done(allocation_started.handle)) {
    return 197;
  }
  destroy_coroutine(allocation_started.handle);
  if (allocation_failure_operator_delete_count != 1) {
    return 198;
  }

  Task typed_handle_started = coroutine_typed_handle_await_suspend();
  if (typed_handle_started.value != 0 ||
      typed_handle_started.handle == 0 ||
      typed_handle_suspend_count != 1 ||
      typed_handle_address != typed_handle_started.handle ||
      typed_handle_done_at_suspend != 0) {
    return 199;
  }
  std::coroutine_handle<Promise> typed_handle =
      std::coroutine_handle<Promise>::from_address(typed_handle_started.handle);
  if (typed_handle.address() != typed_handle_started.handle ||
      typed_handle.done()) {
    return 200;
  }
  typed_handle.resume();
  if (last_resume_value != 429 ||
      frame_state(typed_handle_started.handle) != 0 ||
      !typed_handle.done()) {
    return 201;
  }
  typed_handle.destroy();

  FromPromiseTask from_promise_started = coroutine_from_promise_handle();
  if (from_promise_started.value != 0 ||
      from_promise_started.handle == 0 ||
      from_promise_operator_new_count != 1 ||
      from_promise_operator_delete_count != 0 ||
      frame_state(from_promise_started.handle) != 1) {
    return 202;
  }
  std::coroutine_handle<FromPromisePromise> from_promise_handle =
      std::coroutine_handle<FromPromisePromise>::from_address(
          from_promise_started.handle);
  if (from_promise_handle.address() != from_promise_started.handle ||
      from_promise_handle.done()) {
    return 203;
  }
  from_promise_handle.resume();
  if (last_resume_value != 958 ||
      from_promise_handle.promise().value != 958 ||
      !from_promise_handle.done()) {
    return 204;
  }
  from_promise_handle.destroy();
  if (from_promise_operator_delete_count != 1) {
    return 205;
  }

  destroy_coroutine(started.handle);
  destroy_coroutine(yielded.handle);
  destroy_coroutine(initially_suspended.handle);
  if (promise_coroutine_operator_new_count != 122 ||
      promise_coroutine_operator_delete_count != 122 ||
      global_coroutine_operator_new_count != 4 ||
      global_coroutine_operator_delete_count != 4) {
    return 44;
  }

  return 0;
}
