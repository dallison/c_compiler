#!/usr/bin/env bash

set -euo pipefail

davecc=$1
elfdump=$2

work=${TEST_TMPDIR:-$(mktemp -d)}
mkdir -p "$work"
if [[ -z "${TEST_TMPDIR:-}" ]]; then
  trap 'rm -rf "$work"' EXIT
fi

cat >"$work/eh_frame_smoke.c" <<'EOF'
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

obj="$work/eh_frame_smoke.o"
exe="$work/eh_frame_smoke.exe"

"$davecc" -target x86_64 -O1 -c "$work/eh_frame_smoke.c" -o "$obj"
"$davecc" -target x86_64 -O1 -static -Wl,-e -Wl,main \
  "$work/eh_frame_smoke.c" -o "$exe"

"$elfdump" -S "$obj" >"$work/obj.sections"
"$elfdump" -r "$obj" >"$work/obj.relocs"
"$elfdump" -S "$exe" >"$work/exe.sections"
"$elfdump" -s "$exe" >"$work/exe.symbols"

if ! grep -F ".eh_frame" "$work/obj.sections" >/dev/null; then
  echo "object is missing .eh_frame" >&2
  sed -n '1,120p' "$work/obj.sections" >&2
  exit 1
fi

if ! grep -F ".rela.eh_frame" "$work/obj.relocs" >/dev/null; then
  echo "object is missing .eh_frame relocations" >&2
  sed -n '1,160p' "$work/obj.relocs" >&2
  exit 1
fi

if ! grep -F ".eh_frame" "$work/exe.sections" >/dev/null; then
  echo "linked executable is missing .eh_frame" >&2
  sed -n '1,120p' "$work/exe.sections" >&2
  exit 1
fi

if ! grep -F "__eh_frame_start" "$work/exe.symbols" >/dev/null; then
  echo "linked executable is missing __eh_frame_start" >&2
  sed -n '1,160p' "$work/exe.symbols" >&2
  exit 1
fi

if ! grep -F "__eh_frame_end" "$work/exe.symbols" >/dev/null; then
  echo "linked executable is missing __eh_frame_end" >&2
  sed -n '1,160p' "$work/exe.symbols" >&2
  exit 1
fi
