#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" || "$(uname -m)" != "arm64" ]]; then
  echo "macos native object test requires Apple Silicon" >&2
  exit 1
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
LIBC="$ROOT/$2"
WORK="$(mktemp -d "${TEST_TMPDIR:-/tmp}/macos-native.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
LIBDIR="$WORK/lib"
mkdir -p "$LIBDIR"
cp "$LIBC" "$LIBDIR/libcaarch64_darwin.a"
export DAVECC_INCLUDE_DIR="$ROOT/libc/include"
export DAVECC_LIB_DIR="$LIBDIR"

cat >"$WORK/leaf.c" <<'SRC'
int main(void) { return 42; }
SRC

cat >"$WORK/hello.c" <<'SRC'
int puts(const char*);
int main(void) {
  puts("ok");
  return 0;
}
SRC

"$DAVECC" -target aarch64 -fnative -nostdinc -nostdlib -c "$WORK/leaf.c" \
  -o "$WORK/leaf.o"

magic=$(xxd -p -l 4 "$WORK/leaf.o")
if [[ "$magic" != "cffaedfe" ]]; then
  echo "expected Mach-O MH_MAGIC_64, got $magic" >&2
  file "$WORK/leaf.o" >&2
  exit 1
fi
if ! nm -g "$WORK/leaf.o" | grep -q ' _main$'; then
  echo "Mach-O object is missing _main" >&2
  nm -g "$WORK/leaf.o" >&2
  exit 1
fi

"$DAVECC" -target aarch64 -fnative -nostdinc -nostdlib "$WORK/leaf.c" \
  -o "$WORK/leaf"
status=0
"$WORK/leaf" || status=$?
if [[ "$status" != "42" ]]; then
  echo "native leaf program exited $status, expected 42" >&2
  exit 1
fi

"$DAVECC" -target aarch64 -fnative -nostdinc -nostdlib "$WORK/hello.c" \
  -o "$WORK/hello"
if [[ "$("$WORK/hello")" != "ok" ]]; then
  echo "native hello program did not print ok" >&2
  exit 1
fi

if "$DAVECC" -target x86_64 -fnative -nostdinc -nostdlib -c "$WORK/leaf.c" \
    -o "$WORK/bad.o" >"$WORK/reject.log" 2>&1; then
  echo "expected -fnative to reject non-AArch64 targets" >&2
  cat "$WORK/reject.log" >&2
  exit 1
fi
if ! grep -q 'AArch64' "$WORK/reject.log"; then
  echo "missing AArch64 diagnostic" >&2
  cat "$WORK/reject.log" >&2
  exit 1
fi

cat >"$WORK/cxx.cc" <<'SRC'
#include <iostream>
#include <vector>
int main() {
  std::vector<int> numbers;
  for (int i = 1; i <= 10; ++i) {
    numbers.push_back(i);
  }
  for (int n : numbers) {
    std::cout << n << '\n';
  }
  return 0;
}
SRC

"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++20 \
  "$WORK/cxx.cc" -o "$WORK/cxx"
got="$("$WORK/cxx")"
expect="$(printf '1\n2\n3\n4\n5\n6\n7\n8\n9\n10')"
if [[ "$got" != "$expect" ]]; then
  echo "Darwin C++ program printed:" >&2
  echo "$got" >&2
  exit 1
fi

cat >"$WORK/fs.cc" <<'SRC'
#include <filesystem>
int main() {
  std::filesystem::path here = std::filesystem::current_path();
  if (!std::filesystem::exists(here) || !std::filesystem::is_directory(here)) {
    return 2;
  }
  std::filesystem::path scratch = here / "davecc_fs_scratch";
  std::filesystem::create_directory(scratch);
  if (!std::filesystem::is_directory(scratch)) {
    return 3;
  }
  std::filesystem::remove(scratch);
  return std::filesystem::exists(scratch) ? 4 : 0;
}
SRC

cat >"$WORK/random_clock.cc" <<'SRC'
#include <chrono>
#include <random>
int main() {
  std::random_device device;
  (void)device();
  (void)std::chrono::system_clock::now();
  (void)std::chrono::steady_clock::now();
  return 0;
}
SRC

cat >"$WORK/tz.cc" <<'SRC'
#include <chrono>
#include <iostream>
#include <string>
int main() {
  const std::chrono::time_zone* zone = std::chrono::current_zone();
  if (zone == nullptr || zone->name().empty()) {
    return 3;
  }
  std::cout << "ok " << std::string(zone->name()) << '\n';
  return 0;
}
SRC

cat >"$WORK/threads.cc" <<'SRC'
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread>
int main() {
  if (std::thread::hardware_concurrency() == 0) return 2;
  std::mutex mutex;
  std::condition_variable ready;
  int count = 0;
  std::thread::id main_id = std::this_thread::get_id();
  std::thread::id worker_id;
  std::thread worker([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::lock_guard<std::mutex> lock(mutex);
    worker_id = std::this_thread::get_id();
    count = 1;
    ready.notify_one();
  });
  {
    std::unique_lock<std::mutex> lock(mutex);
    ready.wait(lock, [&] { return count == 1; });
  }
  worker.join();
  if (worker_id == main_id) return 3;

  std::atomic<int> flag{0};
  std::jthread waiter([&](std::stop_token stop) {
    flag.wait(0);
    while (!stop.stop_requested()) std::this_thread::yield();
  });
  flag.store(1);
  flag.notify_one();
  waiter.request_stop();
  waiter.join();
  return 0;
}
SRC

"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++20 \
  "$WORK/threads.cc" -o "$WORK/threads"
if ! "$WORK/threads"; then
  echo "Darwin threads program failed" >&2
  exit 1
fi

"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++20 \
  "$WORK/fs.cc" -o "$WORK/fs"
if ! "$WORK/fs"; then
  echo "Darwin filesystem program failed" >&2
  exit 1
fi

"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++20 \
  "$WORK/random_clock.cc" -o "$WORK/random_clock"
if ! "$WORK/random_clock"; then
  echo "Darwin random/clock program failed" >&2
  exit 1
fi

"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++20 \
  "$WORK/tz.cc" -o "$WORK/tz"
tz="$("$WORK/tz")"
if [[ "$tz" != ok* ]]; then
  echo "Darwin TZDB program printed:" >&2
  echo "$tz" >&2
  exit 1
fi

"$DAVECC" -target aarch64-apple-darwin-davecc -nostdinc -nostdlib -c \
  "$WORK/leaf.c" -o "$WORK/triple.o"
triple_magic=$(xxd -p -l 4 "$WORK/triple.o")
if [[ "$triple_magic" != "cffaedfe" ]]; then
  echo "Darwin triple did not write Mach-O, got $triple_magic" >&2
  exit 1
fi

# Host default (no -target) must pick AArch64 Darwin on this machine.
"$DAVECC" -nostdinc -nostdlib -c "$WORK/leaf.c" -o "$WORK/host.o"
host_magic=$(xxd -p -l 4 "$WORK/host.o")
if [[ "$host_magic" != "cffaedfe" ]]; then
  echo "host default target did not write Mach-O, got $host_magic" >&2
  exit 1
fi
"$DAVECC" "$WORK/leaf.c" -o "$WORK/host_leaf"
status=0
"$WORK/host_leaf" || status=$?
if [[ "$status" != "42" ]]; then
  echo "host default program exited $status, expected 42" >&2
  exit 1
fi

# An object holding only thread-local definitions must not describe its
# synthetic section symbol as a TLV (ld: invalid r_symbolnum).
cat >"$WORK/tlv_only.cc" <<'SRC'
enum class Mode : int { kDefault = 0, kDebug = 1 };
struct Flags { static __thread Mode mode; static thread_local int depth; };
__thread Mode Flags::mode = Mode::kDebug;
thread_local int Flags::depth = 7;
SRC
cat >"$WORK/tlv_main.cc" <<'SRC'
enum class Mode : int { kDefault = 0, kDebug = 1 };
struct Flags { static __thread Mode mode; static thread_local int depth; };
int main() { return Flags::mode == Mode::kDebug && Flags::depth == 7 ? 0 : 1; }
SRC
"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++17 -c \
  "$WORK/tlv_only.cc" -o "$WORK/tlv_only.o"
"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++17 -c \
  "$WORK/tlv_main.cc" -o "$WORK/tlv_main.o"
"$DAVECC" -target aarch64-apple-darwin-davecc "$WORK/tlv_main.o" \
  "$WORK/tlv_only.o" -o "$WORK/tlv"
if ! "$WORK/tlv"; then
  echo "thread-local-only object program failed" >&2
  exit 1
fi

# An out-of-line `= default` special member is a strong definition even when
# its own translation unit never uses it.
cat >"$WORK/defaulted.h" <<'SRC'
#include <string>
namespace defaulted_ns {
class Table {
 public:
  Table();
  ~Table();
  std::string name;
};
}
SRC
cat >"$WORK/defaulted_def.cc" <<'SRC'
#include "defaulted.h"
namespace defaulted_ns {
Table::Table() = default;
Table::~Table() = default;
}
SRC
cat >"$WORK/defaulted_main.cc" <<'SRC'
#include "defaulted.h"
int main() {
  defaulted_ns::Table table;
  table.name = "ok";
  return table.name.size() == 2 ? 0 : 1;
}
SRC
"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++17 -c \
  "$WORK/defaulted_def.cc" -o "$WORK/defaulted_def.o"
"$DAVECC" -target aarch64-apple-darwin-davecc -std=c++17 -c \
  "$WORK/defaulted_main.cc" -o "$WORK/defaulted_main.o"
"$DAVECC" -target aarch64-apple-darwin-davecc "$WORK/defaulted_main.o" \
  "$WORK/defaulted_def.o" -o "$WORK/defaulted"
if ! "$WORK/defaulted"; then
  echo "out-of-line defaulted member program failed" >&2
  exit 1
fi

# abseil's LowLevelAlloc: libSystem's sysconf and mmap take Darwin's names
# and flags, and a narrower integer converted to a pointer extends by its
# signedness, so MAP_FAILED compares equal to what a failed mmap returns.
cat >"$WORK/posix_values.c" <<'SRC'
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
__attribute__((noinline)) static int minus_one(void) {
  volatile int v = -1;
  return v;
}
__attribute__((noinline)) static void* failed_pointer(void) {
  volatile intptr_t v = -1;
  return (void*)v;
}
int main(void) {
  long page = sysconf(_SC_PAGESIZE);
  if (page <= 0 || page != getpagesize()) return 1;
  if (sysconf(_SC_NPROCESSORS_ONLN) <= 0) return 2;
  void* p = mmap(0, (size_t)page, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (p == MAP_FAILED) return 3;
  *(volatile char*)p = 1;
  munmap(p, (size_t)page);
  if (failed_pointer() != MAP_FAILED) return 4;
  if ((uintptr_t)(void*)minus_one() != UINTPTR_MAX) return 5;
  if ((uintptr_t)(void*)(unsigned)minus_one() != 0xffffffffu) return 6;
  return 0;
}
SRC
"$DAVECC" -target aarch64-apple-darwin-davecc "$WORK/posix_values.c" \
  -o "$WORK/posix_values"
status=0
"$WORK/posix_values" || status=$?
if [[ "$status" != "0" ]]; then
  echo "Darwin sysconf/mmap/pointer-conversion program exited $status" >&2
  exit 1
fi

# libSystem's shm_open is variadic, so its mode travels on the stack.
cat >"$WORK/shm.c" <<'SRC'
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
int main(void) {
  char name[64];
  snprintf(name, sizeof(name), "/davecc_shm_%d", (int)getpid());
  mode_t old_umask = umask(0);
  int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  umask(old_umask);
  if (fd < 0) return 1;
  struct stat st;
  int mode_ok = fstat(fd, &st) == 0 && (st.st_mode & 0777) == 0600;
  int again = shm_open(name, O_RDWR, 0);
  shm_unlink(name);
  close(fd);
  if (again < 0) return 2;
  close(again);
  return mode_ok ? 0 : 3;
}
SRC
"$DAVECC" -target aarch64-apple-darwin-davecc "$WORK/shm.c" -o "$WORK/shm"
status=0
"$WORK/shm" || status=$?
if [[ "$status" != "0" ]]; then
  echo "Darwin shm_open program exited $status" >&2
  exit 1
fi
