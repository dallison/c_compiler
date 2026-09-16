#!/bin/bash
set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "usage: $0 <davecc> <bpf_interpreter>" >&2
  exit 2
fi

davecc=$1
bpf=$2

if [ -f "libc/include/stdio.h" ]; then
  :
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/.."
fi

work=$(mktemp -d "${TMPDIR:-/tmp}/bpf_runtime_smoke.XXXXXX")
trap 'rm -rf "$work"' EXIT

cat > "$work/return_const.c" <<'SRC'
int main(void) {
  return 42;
}
SRC

"$davecc" -target bpf-unknown-linux-davecc -nostdlib -nostdinc -static -Wl,-e -Wl,main \
  "$work/return_const.c" -o "$work/return_const.exe"

set +e
"$bpf" "$work/return_const.exe"
rc=$?
set -e
if [ "$rc" -ne 42 ]; then
  echo "expected exit 42, got $rc" >&2
  exit 1
fi

cat > "$work/add.c" <<'SRC'
int add(int a, int b) {
  return a + b;
}

int main(void) {
  return add(20, 22);
}
SRC

"$davecc" -target bpf-unknown-linux-davecc -nostdlib -nostdinc -static -Wl,-e -Wl,main \
  "$work/add.c" -o "$work/add.exe"

set +e
"$bpf" "$work/add.exe"
rc=$?
set -e
if [ "$rc" -ne 42 ]; then
  echo "expected add() exit 42, got $rc" >&2
  exit 1
fi

cat > "$work/local.c" <<'SRC'
int main(void) {
  int x = 17;
  int y = 25;
  return x + y;
}
SRC

"$davecc" -target bpf-unknown-linux-davecc -nostdlib -nostdinc -static -Wl,-e -Wl,main \
  "$work/local.c" -o "$work/local.exe"

set +e
"$bpf" "$work/local.exe"
rc=$?
set -e
if [ "$rc" -ne 42 ]; then
  echo "expected local exit 42, got $rc" >&2
  exit 1
fi
