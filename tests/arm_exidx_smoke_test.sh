#!/usr/bin/env bash

set -euo pipefail

davecc=$1
elfdump=$2

work=${TEST_TMPDIR:-$(mktemp -d)}
mkdir -p "$work"
if [[ -z "${TEST_TMPDIR:-}" ]]; then
  trap 'rm -rf "$work"' EXIT
fi

cat >"$work/arm_exidx_smoke.c" <<'EOF'
int callee(int value) {
  return value + 1;
}

int caller(int value) {
  return callee(value) + 2;
}

int main(void) {
  return caller(3) == 6 ? 0 : 1;
}
EOF

obj="$work/arm_exidx_smoke.o"
exe="$work/arm_exidx_smoke.exe"

"$davecc" -target arm -O1 -nostdinc -c "$work/arm_exidx_smoke.c" -o "$obj"
"$davecc" -target arm -O1 -nostdinc -nostdlib -static -Wl,-e -Wl,main \
  "$work/arm_exidx_smoke.c" -o "$exe"

"$elfdump" -S "$obj" >"$work/obj.sections"
"$elfdump" -S "$exe" >"$work/exe.sections"
strings "$exe" >"$work/exe.strings"

if ! grep -F ".ARM.exidx" "$work/obj.sections" >/dev/null; then
  echo "object is missing .ARM.exidx" >&2
  sed -n '1,120p' "$work/obj.sections" >&2
  exit 1
fi

if ! grep -F ".ARM.extab" "$work/obj.sections" >/dev/null; then
  echo "object is missing .ARM.extab" >&2
  sed -n '1,120p' "$work/obj.sections" >&2
  exit 1
fi

if ! grep -F ".rela.ARM.exidx" "$work/obj.sections" >/dev/null; then
  echo "object is missing .ARM.exidx relocations" >&2
  sed -n '1,160p' "$work/obj.sections" >&2
  exit 1
fi

if ! grep -F "ARM_EXIDX" "$work/exe.sections" >/dev/null; then
  echo "linked executable is missing .ARM.exidx" >&2
  sed -n '1,120p' "$work/exe.sections" >&2
  exit 1
fi

if ! grep -F "__exidx_start" "$work/exe.strings" >/dev/null; then
  echo "linked executable is missing __exidx_start" >&2
  sed -n '1,160p' "$work/exe.strings" >&2
  exit 1
fi

if ! grep -F "__exidx_end" "$work/exe.strings" >/dev/null; then
  echo "linked executable is missing __exidx_end" >&2
  sed -n '1,160p' "$work/exe.strings" >&2
  exit 1
fi

if ! grep -F "__extab_start" "$work/exe.strings" >/dev/null; then
  echo "linked executable is missing __extab_start" >&2
  sed -n '1,160p' "$work/exe.strings" >&2
  exit 1
fi

if ! grep -F "__extab_end" "$work/exe.strings" >/dev/null; then
  echo "linked executable is missing __extab_end" >&2
  sed -n '1,160p' "$work/exe.strings" >&2
  exit 1
fi

if ! grep -F "__davecc_arm_unwind_fp" "$work/exe.strings" >/dev/null; then
  echo "linked executable is missing __davecc_arm_unwind_fp" >&2
  sed -n '1,160p' "$work/exe.strings" >&2
  exit 1
fi
