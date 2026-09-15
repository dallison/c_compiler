#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <random>
#include <sys/mman.h>
#include <sys/wait.h>
#include <threads.h>
#include <unistd.h>

thread_local int native_tls_value = 17;
static int global_constructor_ran;
static int thread_destructor_count;
static thrd_t observed_thread;
static const char* destructor_path;

struct GlobalLifecycleProbe {
  GlobalLifecycleProbe() { global_constructor_ran = 1; }
  ~GlobalLifecycleProbe() {
    if (destructor_path == nullptr) return;
    FILE* marker = fopen(destructor_path, "w");
    if (marker != nullptr) {
      fputs("destructor-ran\n", marker);
      fclose(marker);
    }
  }
} global_lifecycle_probe;

struct ThreadLifecycleProbe {
  ~ThreadLifecycleProbe() { ++thread_destructor_count; }
};

thread_local ThreadLifecycleProbe thread_lifecycle_probe;

struct ThreadContext {
  mtx_t mutex;
  int value;
};

extern "C" int RunThread(void* argument) {
  if (native_tls_value != 17) return 1;
  (void)thread_lifecycle_probe;
  native_tls_value = 29;
  if (native_tls_value != 29) return 2;
  observed_thread = thrd_current();
  ThreadContext* context = (ThreadContext*)argument;
  if (mtx_lock(&context->mutex) != thrd_success) return 3;
  ++context->value;
  if (mtx_unlock(&context->mutex) != thrd_success) return 4;
  return 0;
}

extern "C" int ExitThread(void*) {
  thrd_exit(7);
  return 0;
}

extern "C" int HeapThread(void*) {
  for (int index = 0; index < 1000; ++index) {
    size_t size = (size_t)(index % 257) + 1;
    unsigned char* memory = (unsigned char*)malloc(size);
    if (memory == nullptr) return 1;
    memory[0] = (unsigned char)index;
    memory[size - 1] = (unsigned char)(index >> 1);
    free(memory);
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc > 1) destructor_path = argv[1];
  if (global_constructor_ran != 1) return 1;

#if defined(__risc_v__) && defined(__ILP32__)
  volatile double wide_fp = 4294967297.0;
  volatile double wide_unsigned_fp = 9223372036854775808.0;
  volatile long long wide_positive = 4294967297LL;
  volatile long long wide_negative = -4294967297LL;
  volatile unsigned long long wide_unsigned = 9223372036854775808ULL;
  if ((long long)wide_fp != 4294967297LL) return 44;
  if ((double)wide_positive != 4294967297.0) return 45;
  if ((double)wide_negative != -4294967297.0) return 46;
  char wide_format[32];
  if (snprintf(wide_format, sizeof(wide_format), "%lld", wide_negative) < 0 ||
      strcmp(wide_format, "-4294967297") != 0)
    return 47;
  if ((unsigned long long)wide_unsigned_fp != 9223372036854775808ULL)
    return 48;
  if ((double)wide_unsigned != 9223372036854775808.0) return 49;
#endif

  errno = 0;
  FILE* missing = fopen("/definitely/not/a/davecc/file", "r");
  if (missing != nullptr || errno != ENOENT) return 10;

  const char* file_path = "/tmp/davecc-native-linux-smoke.txt";
  FILE* output = fopen(file_path, "w");
  if (output == nullptr || fputs("native-linux\n", output) < 0 ||
      fclose(output) != 0)
    return 19;
  FILE* input = fopen(file_path, "r");
  char buffer[32]{};
  if (input == nullptr || fgets(buffer, sizeof(buffer), input) == nullptr ||
      fclose(input) != 0)
    return 20;

  int descriptor = open(file_path, O_RDONLY);
  char first = '\0';
  if (descriptor < 0) return 34;
  if (pread(descriptor, &first, 1, 0) != 1 || first != 'n') return 40;
  if (pread(descriptor, &first, 1, 1) != 1) return 41;
  if (first != 'a') return 43;
  if (close(descriptor) != 0) return 42;
  char cwd_buffer[4096];
  if (getcwd(cwd_buffer, sizeof(cwd_buffer)) == nullptr ||
      cwd_buffer[0] != '/' || getenv("PATH") == nullptr)
    return 35;

  std::error_code remove_error;
  if (!std::filesystem::remove(file_path, remove_error) || remove_error)
    return 21;

  void* memory = malloc(8192);
  if (memory == nullptr) return 11;
  free(memory);

  if (sysconf(_SC_PAGESIZE) != 4096) return 36;
  void* mapping = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping == MAP_FAILED) return 37;
  ((unsigned char*)mapping)[0] = 0x5a;
  if (mprotect(mapping, 4096, PROT_READ) != 0 ||
      ((unsigned char*)mapping)[0] != 0x5a ||
      munmap(mapping, 4096) != 0)
    return 38;

  pid_t child = fork();
  if (child == 0) _exit(42);
  int child_status = 0;
  if (child < 0 || waitpid(child, &child_status, 0) != child ||
      !WIFEXITED(child_status) || WEXITSTATUS(child_status) != 42)
    return 39;

  void* allocations[4096]{};
  for (int index = 0; index < 4096; ++index) {
    allocations[index] = malloc(512);
    if (allocations[index] == nullptr ||
        ((uintptr_t)allocations[index] & 15) != 0)
      return 29;
  }
  for (int index = 0; index < 4096; ++index) {
    free(allocations[index]);
  }
  unsigned char* large_allocation = (unsigned char*)malloc(2 * 1024 * 1024);
  if (large_allocation == nullptr) return 30;
  large_allocation[0] = 0x5a;
  large_allocation[2 * 1024 * 1024 - 1] = 0xa5;
  if (large_allocation[0] != 0x5a ||
      large_allocation[2 * 1024 * 1024 - 1] != 0xa5)
    return 31;
  free(large_allocation);

  timespec value{};
  if (timespec_get(&value, TIME_UTC) != TIME_UTC || value.tv_sec <= 0)
    return 12;

  std::random_device random;
  if (random.entropy() == 0.0) return 13;
  (void)random();

  std::error_code error;
  std::filesystem::path cwd = std::filesystem::current_path(error);
  if (error || cwd.empty()) return 14;
  if (std::chrono::locate_zone("UTC") == nullptr) return 22;
  try {
    throw 23;
  } catch (int value) {
    if (value != 23) return 23;
  }
  if (native_tls_value != 17) return 18;

  ThreadContext context{};
  if (mtx_init(&context.mutex, mtx_plain) != thrd_success) return 24;
  thrd_t thread{};
  if (thrd_create(&thread, RunThread, &context) != thrd_success) return 15;
  int result = -1;
  if (thrd_join(thread, &result) != thrd_success) return 16;
  if (result != 0) return 20 + result;
  if (observed_thread != thread || context.value != 1) return 25;
  if (thread_destructor_count != 1) return 26;
  mtx_destroy(&context.mutex);
  if (native_tls_value != 17) return 17;

  thrd_t exit_thread{};
  if (thrd_create(&exit_thread, ExitThread, nullptr) != thrd_success) return 27;
  result = -1;
  if (thrd_join(exit_thread, &result) != thrd_success || result != 7) return 28;

  thrd_t heap_threads[4]{};
  for (int index = 0; index < 4; ++index) {
    if (thrd_create(&heap_threads[index], HeapThread, nullptr) != thrd_success)
      return 32;
  }
  for (int index = 0; index < 4; ++index) {
    result = -1;
    if (thrd_join(heap_threads[index], &result) != thrd_success || result != 0)
      return 33;
  }
  return 0;
}
