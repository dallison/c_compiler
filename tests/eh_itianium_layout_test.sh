#!/usr/bin/env bash

set -euo pipefail

davecc=$1
elfdump=$2
target=$3

if [[ -f "tests/eh_itianium_layout_test.sh" ]]; then
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

cat >"$work/eh_layout.cpp" <<'EOF'
struct E { E() {} ~E() {} };
int helper() {
  E obj;
  try {
    throw 7;
  } catch (int value) {
    return value;
  }
  return 0;
}
int main() {
  return helper() == 7 ? 0 : 1;
}
EOF

obj="$work/eh_layout.o"
"$davecc" -target "$target" -fexceptions -O1 -isystem libc/include \
  -c "$work/eh_layout.cpp" -o "$obj"

"$elfdump" -S "$obj" >"$work/sections"
"$elfdump" -s "$obj" >"$work/symbols"
"$elfdump" -r "$obj" >"$work/relocs"

grep -F ".eh_frame" "$work/sections" >/dev/null
grep -F ".gcc_except_table" "$work/sections" >/dev/null
grep -F "__gxx_personality_v0" "$work/symbols" >/dev/null

if [[ "$target" == "x86_64" ]]; then
  grep -F ".rela.eh_frame" "$work/relocs" >/dev/null
fi
