#!/bin/bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "usage: $0 <davecc> <libc_wasm32> [wasm32run]" >&2
  exit 2
fi

davecc=$1
libc=$2
runner=${3:-}

if [ -f "libc/include/stdio.h" ]; then
  :
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/.."
fi

if [ -z "$runner" ]; then
  if [ -x tools/wasm32run.sh ]; then
    runner=tools/wasm32run.sh
  else
    runner=$(command -v wasmtime || true)
  fi
fi
if [ -z "$runner" ]; then
  echo "wasm32_runtime_smoke_test: wasmtime is not installed" >&2
  exit 2
fi

export DAVECC_INCLUDE_DIR="${DAVECC_INCLUDE_DIR:-$PWD/libc/include}"
export DAVECC_LIB_DIR="${DAVECC_LIB_DIR:-$(dirname "$libc")}"

work=$(mktemp -d "${TMPDIR:-/tmp}/wasm32_runtime_smoke.XXXXXX")
trap 'rm -rf "$work"' EXIT

cat > "$work/printf_smoke.c" <<'SRC'
#include <stdio.h>

int main(void) {
  printf("hello wasm32\n");
  return 0;
}
SRC

"$davecc" -target wasm32 -static \
  "$work/printf_smoke.c" -isystem libc/include \
  -o "$work/printf_smoke.wasm" "$libc"

if command -v wasm-validate >/dev/null 2>&1; then
  wasm-validate "$work/printf_smoke.wasm"
fi

if [ "$(basename "$runner")" = "wasmtime" ]; then
  "$runner" run --dir "${PWD}::/" "$work/printf_smoke.wasm" > "$work/output.txt"
else
  "$runner" "$work/printf_smoke.wasm" > "$work/output.txt"
fi
printf 'hello wasm32\n' > "$work/expected.txt"
cmp "$work/expected.txt" "$work/output.txt"

cat > "$work/file_smoke.c" <<'SRC'
#include <stdio.h>

int main(void) {
  const char* path = "/tmp/davecc-wasm32-smoke.txt";
  FILE* file = fopen(path, "w+");
  if (file == NULL) return 1;
  if (fputs("ok\n", file) < 0) return 2;
  if (fseek(file, 0, SEEK_SET) != 0) return 3;
  char buf[8];
  if (fgets(buf, sizeof(buf), file) == NULL) return 4;
  if (fclose(file) != 0) return 5;
  if (remove(path) != 0) return 6;
  printf("%s", buf);
  return 0;
}
SRC

"$davecc" -target wasm32 -static \
  "$work/file_smoke.c" -isystem libc/include \
  -o "$work/file_smoke.wasm" "$libc"

if [ "$(basename "$runner")" = "wasmtime" ]; then
  "$runner" run --dir "${PWD}::/" --dir /tmp::/tmp "$work/file_smoke.wasm" \
    > "$work/file_output.txt"
else
  "$runner" "$work/file_smoke.wasm" > "$work/file_output.txt"
fi
printf 'ok\n' > "$work/file_expected.txt"
cmp "$work/file_expected.txt" "$work/file_output.txt"

cat > "$work/setjmp_smoke.c" <<'SRC'
#include <setjmp.h>

static int secondary_completed;

static void jump_through_frames(jmp_buf env, int depth) {
  volatile int marker = depth + 1;
  if (depth != 0) {
    jump_through_frames(env, depth - 1);
  }
  if (marker == 1) {
    longjmp(env, 77);
  }
}

int main(void) {
  jmp_buf primary;
  jmp_buf secondary;
  int value = setjmp(primary);
  if (value == 0) {
    longjmp(primary, 0);
  }
  if (value != 1) return 1;

  value = setjmp(primary);
  if (value == 0) {
    jump_through_frames(primary, 4);
  }
  if (value != 77) return 2;

  value = setjmp(primary);
  if (value == 0) {
    int secondary_value = setjmp(secondary);
    if (secondary_value == 0) {
      longjmp(secondary, 19);
    }
    if (secondary_value != 19) return 3;
    secondary_completed = 1;
    longjmp(primary, -7);
  }
  if (value != -7) return 4;
  if (!secondary_completed) return 5;
  return 0;
}
SRC

"$davecc" -target wasm32 -static \
  "$work/setjmp_smoke.c" -isystem libc/include \
  -o "$work/setjmp_smoke.wasm" "$libc"

if [ "$(basename "$runner")" = "wasmtime" ]; then
  "$runner" run -W exceptions --dir "${PWD}::/" "$work/setjmp_smoke.wasm"
else
  "$runner" "$work/setjmp_smoke.wasm"
fi
