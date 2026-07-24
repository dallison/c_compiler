#!/usr/bin/env bash

set -euo pipefail

davecc=$1
interpreter=$2
libc=$3
target=$4

if [[ -f "libc/tests/run_tests.sh" ]]; then
  :
elif [[ -n "${TEST_SRCDIR:-}" && -n "${TEST_WORKSPACE:-}" ]]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/.."
fi

work=${TEST_TMPDIR:-$(mktemp -d)}
mkdir -p "$work"
if [[ -z "${TEST_TMPDIR:-}" ]]; then
  trap 'rm -rf "$work"' EXIT
fi

cat >"$work/eh_runtime.cpp" <<'EOF'
struct Pod { int value; };

int catch_int(void) {
  try {
    throw 42;
  } catch (int value) {
    return value;
  }
}

int catch_all(void) {
  try {
    throw 1;
  } catch (...) {
    return 0;
  }
}

int ordered_handlers(void) {
  try {
    throw 23;
  } catch (char) {
    return -1;
  } catch (int value) {
    return value;
  } catch (...) {
    return -2;
  }
}

int cleanup_runs = 0;
struct CleanupProbe {
  ~CleanupProbe() { cleanup_runs++; }
};

int cleanup_then_catch(void) {
  cleanup_runs = 0;
  try {
    CleanupProbe probe;
    throw 3;
  } catch (int value) {
    return value + cleanup_runs;
  }
}

int rethrow_once(void) {
  try {
    try {
      throw 9;
    } catch (int) {
      throw;
    }
  } catch (int value) {
    return value;
  }
}

int catch_pod(void) {
  try {
    throw Pod{11};
  } catch (const Pod& caught) {
    return caught.value;
  }
}

int main() {
  if (catch_int() != 42) {
    return 1;
  }
  if (catch_all() != 0) {
    return 2;
  }
  if (ordered_handlers() != 23) {
    return 6;
  }
  if (cleanup_then_catch() != 4) {
    return 3;
  }
  if (rethrow_once() != 9) {
    return 4;
  }
  if (catch_pod() != 11) {
    return 5;
  }
  return 0;
}
EOF

exe="$work/eh_runtime.exe"
"$davecc" -target "$target" -fexceptions -O1 -static -isystem libc/include \
  -Wl,-e -Wl,main "$work/eh_runtime.cpp" "$libc" -o "$exe"
"$interpreter" "$exe"
