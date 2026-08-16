import std;

void set_threaded_value(int* value) {
  *value = 1;
}

int module_threaded_value = 0;
int module_jthreaded_value = 0;

void set_module_threaded_value() {
  module_threaded_value = 1;
}

void set_module_jthreaded_value() {
  module_jthreaded_value = 1;
}

static_assert(std::is_pointer<decltype(&set_threaded_value)>::value);
static_assert(std::is_same_v<int, int>);
static_assert(
    std::is_invocable<decltype(&set_threaded_value), int*>::value);
static_assert(std::mdspan<int, std::extents<std::size_t, 2, 3>>::rank() == 2);
static_assert(std::is_same_v<decltype(1.0f32), std::float32_t>);
static_assert(__cpp_lib_stacktrace == 202011L);
static_assert(std::is_same_v<std::stacktrace::value_type,
                             std::stacktrace_entry>);
#if defined(__STDCPP_FLOAT64_T__)
static_assert(std::is_same_v<decltype(1.0f64), std::float64_t>);
#endif

int main() {
  std::array<int, 3> values{2, 3, 5};
  if (values[0] + values[1] != values[2]) {
    return 1;
  }

  int threaded_value = 0;
  std::thread thread(set_module_threaded_value);
  thread.join();
  std::jthread worker(set_module_jthreaded_value);
  worker.join();
  if (thread.joinable() || worker.joinable() ||
      module_threaded_value != 1 || module_jthreaded_value != 1 ||
      threaded_value != 0) {
    return 2;
  }

  std::counting_semaphore<2> semaphore(1);
  semaphore.acquire();
  semaphore.release();
  std::latch latch(1);
  latch.count_down();
  latch.wait();
  std::barrier<> barrier(1);
  barrier.arrive_and_wait();
  std::condition_variable condition;
  condition.notify_all();

  std::stop_source source;
  if (!source.request_stop() || !source.get_token().stop_requested()) {
    return 3;
  }

  int matrix_data[6] = {1, 2, 3, 4, 5, 6};
  std::mdspan matrix(matrix_data, 2, 3);
  if (matrix.extent(0) != 2 || matrix.extent(1) != 3 ||
      matrix[1, 2] != 6) {
    return 4;
  }
  std::float32_t fixed_float = 1.25f32;
  if (fixed_float + 0.75f32 != 2.0f32) {
    return 5;
  }
#if defined(__STDCPP_FLOAT64_T__)
  std::float64_t fixed_double = 2.5f64;
  if (fixed_double + 1.5f64 != 4.0f64) {
    return 6;
  }
#endif
  std::stacktrace trace = std::stacktrace::current(0, 1);
  if (trace.size() != 1 || !trace[0]) {
    return 7;
  }
  return 0;
}
