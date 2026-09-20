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
