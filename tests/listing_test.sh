#!/bin/bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "usage: $0 <davecc>" >&2
  exit 2
fi

davecc=$1
work="${TEST_TMPDIR:-$(mktemp -d "${TMPDIR:-/tmp}/davecc-listing.XXXXXX")}"
mkdir -p "$work"

cat >"$work/add.c" <<'EOF'
int add(int a, int b) {
  return a + b;
}

int main(void) {
  return add(2, 3);
}
EOF

if ! "$davecc" -target x86_64 -c -nostdinc -nostdlib \
  -flisting -flisting-ast -flisting-ir -flisting-lowered \
  -flisting-file="$work/add.lst" \
  "$work/add.c" -o "$work/add.o"; then
  echo "davecc failed to compile listing test" >&2
  exit 1
fi

lst="$work/add.lst"
if [ ! -s "$lst" ]; then
  echo "listing file was not created" >&2
  exit 1
fi

fail() {
  echo "$1" >&2
  echo "---- listing ----" >&2
  cat "$lst" >&2
  exit 1
}

grep -q "DaveCC listing" "$lst" || fail "missing listing header"
grep -q "file $work/add.c" "$lst" || grep -q "file " "$lst" || fail "missing file name"
grep -q "int add(int a, int b)" "$lst" || fail "missing source line"
grep -q "BB " "$lst" || fail "missing basic block markers"
grep -q "AST:" "$lst" || fail "missing AST section"
grep -q "IR:" "$lst" || fail "missing IR section"
grep -q "lowered:" "$lst" || fail "missing lowered IR section"
grep -q "assembly:" "$lst" || fail "missing assembly section"
grep -q "function add" "$lst" || fail "missing function add"
grep -q "function main" "$lst" || fail "missing function main"

# File name should not be repeated on every source line.
file_headers=$(grep -c "^file " "$lst" || true)
if [ "$file_headers" -lt 1 ]; then
  fail "expected at least one file header"
fi
source_lines=$(grep -c " | " "$lst" || true)
if [ "$source_lines" -le "$file_headers" ]; then
  fail "source lines should outnumber file headers"
fi

# Source+assembly listings must not emit empty "BB N:" headers.
"$davecc" -target x86_64 -c -nostdinc -nostdlib \
  -flisting -flisting-file="$work/asm.lst" \
  "$work/add.c" -o "$work/add_asm.o"
python3 - "$work/asm.lst" <<'PY'
import sys
path = sys.argv[1]
lines = open(path).read().splitlines()
for i, line in enumerate(lines):
    if not line.startswith("BB ") or not line.endswith(":"):
        continue
    nxt = lines[i + 1] if i + 1 < len(lines) else ""
    if nxt.startswith("BB ") or nxt.startswith("  assembly:") or nxt == "":
        sys.stderr.write("empty basic-block header %r followed by %r\n" %
                         (line, nxt))
        sys.exit(1)
PY

# IR-only listing should not imply assembly.
"$davecc" -target x86_64 -c -nostdinc -nostdlib \
  -flisting-ir -flisting-file="$work/ir.lst" \
  "$work/add.c" -o "$work/add_ir.o"

grep -q "IR:" "$work/ir.lst" || fail "IR-only listing missing IR"
if grep -q "assembly:" "$work/ir.lst"; then
  fail "IR-only listing should not include assembly"
fi

echo "listing tests passed"
