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
int throws_or_catches() {
  try {
    throw 42;
  } catch (int value) {
    return value;
  }
}
int main() {
  return throws_or_catches() == 42 ? 0 : 1;
}
EOF

exe="$work/eh_runtime.exe"
"$davecc" -target "$target" -fexceptions -O1 -static -isystem libc/include \
  -Wl,-e -Wl,main "$work/eh_runtime.cpp" "$libc" -o "$exe"
"$interpreter" "$exe"
