#!/usr/bin/env bash
set -euo pipefail

davecc="$1"
interpreter="$2"
libc="$3"
elfdump="$4"

work="${TEST_TMPDIR:-$(mktemp -d "${TMPDIR:-/tmp}/davecc-stacktrace-dso.XXXXXX")}"
mkdir -p "$work"

cat >"$work/library.cpp" <<'EOF'
using size_t = unsigned long;
using uintptr_t = unsigned long;

extern "C" size_t __davecc_stacktrace_capture(
    uintptr_t*, size_t, size_t, size_t) {
  return 0;
}

[[gnu::noinline]] static int local_dso_frame() {
  return 42;
}

extern "C" uintptr_t dso_frame_address() {
  return reinterpret_cast<uintptr_t>(&local_dso_frame);
}
EOF

cat >"$work/main.cpp" <<'EOF'
#include <stacktrace_runtime.h>
#include <string>

extern "C" uintptr_t dso_frame_address();

int main() {
  uintptr_t address = dso_frame_address();
  const char* description = __davecc_stacktrace_description(address);
  if (description == nullptr) {
    return 1;
  }
  return std::string(description).find("local_dso_frame") ==
                 std::string::npos
             ? 2
             : 0;
}
EOF

cat >"$work/plain.cpp" <<'EOF'
extern "C" int plain_dso_function() {
  return 7;
}
EOF

"$davecc" -target x86_64 -std=c++23 -O1 -shared \
  "$work/library.cpp" -o "$work/libstackframes.so"
"$davecc" -target x86_64 -std=c++23 -O1 -shared \
  "$work/plain.cpp" -o "$work/libplain.so"
"$davecc" -target x86_64 -std=c++23 -O1 -Wl,-e -Wl,main \
  -rpath "$work" -isystem libc/include "$work/main.cpp" \
  "$work/libstackframes.so" "$libc" -o "$work/stacktrace-dso.exe"

stacktrace_sections=$("$elfdump" -S "$work/libstackframes.so")
plain_sections=$("$elfdump" -S "$work/libplain.so")
case "$stacktrace_sections" in
  *".davecc_stacktrace"*) ;;
  *) echo "stacktrace DSO is missing symbol metadata" >&2; exit 1 ;;
esac
case "$plain_sections" in
  *".davecc_stacktrace"*)
    echo "unused stacktrace metadata was emitted" >&2
    exit 1
    ;;
esac

if [[ "$(uname -s)" == "Darwin" ]]; then
  echo "skipping dynamic DSO execution on Darwin: fixed-address MAP_FIXED is unsupported here" >&2
  exit 0
fi

(
  cd "$work"
  LD_LIBRARY_PATH="$work:${LD_LIBRARY_PATH:-}" \
    "$interpreter" -i stacktrace-dso.exe
)
