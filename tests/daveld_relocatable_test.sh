#!/bin/bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 davecc daveld elfdump x86_64" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
DAVELD="$ROOT/$2"
ELFDUMP="$ROOT/$3"
X86_64="$ROOT/$4"
WORK="${TEST_TMPDIR:-/tmp}/daveld-relocatable"
mkdir -p "$WORK"

"$DAVELD" --help | grep -q -- '-r, --relocatable'

cat >"$WORK/add.c" <<'SRC'
int add(int x, int y) {
  return x + y;
}
SRC

cat >"$WORK/main.c" <<'SRC'
int add(int x, int y);

int main(void) {
  return add(2, 3);
}
SRC

"$DAVECC" -target x86_64 -nostdinc -c "$WORK/add.c" -o "$WORK/add.o"
"$DAVECC" -target x86_64 -nostdinc -c "$WORK/main.c" -o "$WORK/main.o"

"$DAVELD" -r "$WORK/add.o" "$WORK/main.o" -o "$WORK/combined.o"

header=$("$ELFDUMP" -H "$WORK/combined.o")
if ! printf '%s\n' "$header" | grep -q $'Type:\tREL'; then
  echo "daveld -r did not write a relocatable object" >&2
  printf '%s\n' "$header" >&2
  exit 1
fi

symbols=$("$ELFDUMP" -s "$WORK/combined.o")
if ! printf '%s\n' "$symbols" | grep -q add; then
  echo "combined object is missing add" >&2
  printf '%s\n' "$symbols" >&2
  exit 1
fi
if ! printf '%s\n' "$symbols" | grep -q main; then
  echo "combined object is missing main" >&2
  printf '%s\n' "$symbols" >&2
  exit 1
fi

relocs=$("$ELFDUMP" -r "$WORK/combined.o")
if ! printf '%s\n' "$relocs" | grep -q add; then
  echo "combined object dropped the relocation against add" >&2
  printf '%s\n' "$relocs" >&2
  exit 1
fi

"$DAVECC" -target x86_64 -nostdinc -nostdlib -static -e main \
  "$WORK/combined.o" -o "$WORK/combined.exe"
set +e
"$X86_64" -i "$WORK/combined.exe"
status=$?
set -e
if [[ "$status" -ne 5 ]]; then
  echo "combined x86_64 program exited $status, expected 5" >&2
  exit 1
fi

"$DAVELD" -static -e main "$WORK/add.o" "$WORK/main.o" -o "$WORK/direct.exe"
set +e
"$X86_64" -i "$WORK/direct.exe"
status=$?
set -e
if [[ "$status" -ne 5 ]]; then
  echo "daveld executable link exited $status, expected 5" >&2
  exit 1
fi

"$DAVECC" -target x86_64 -nostdinc -r "$WORK/add.c" "$WORK/main.c" \
  -o "$WORK/from_driver.o"
header=$("$ELFDUMP" -H "$WORK/from_driver.o")
if ! printf '%s\n' "$header" | grep -q $'Type:\tREL'; then
  echo "davecc -r did not write a relocatable object" >&2
  printf '%s\n' "$header" >&2
  exit 1
fi
"$DAVECC" -target x86_64 -nostdinc -nostdlib -static -e main \
  "$WORK/from_driver.o" -o "$WORK/from_driver.exe"
set +e
"$X86_64" -i "$WORK/from_driver.exe"
status=$?
set -e
if [[ "$status" -ne 5 ]]; then
  echo "davecc -r program exited $status, expected 5" >&2
  exit 1
fi

"$DAVECC" -target 65c02 -nostdinc -c "$WORK/add.c" -o "$WORK/add65.o"
"$DAVECC" -target 65c02 -nostdinc -c "$WORK/main.c" -o "$WORK/main65.o"
"$DAVELD" -r "$WORK/add65.o" "$WORK/main65.o" -o "$WORK/combined65.o"
header=$("$ELFDUMP" -H "$WORK/combined65.o")
if ! printf '%s\n' "$header" | grep -q $'Type:\tREL'; then
  echo "daveld -r on 65C02 did not write a relocatable object" >&2
  printf '%s\n' "$header" >&2
  exit 1
fi
symbols=$("$ELFDUMP" -s "$WORK/combined65.o")
if ! printf '%s\n' "$symbols" | grep -q add; then
  echo "65C02 combined object is missing add" >&2
  printf '%s\n' "$symbols" >&2
  exit 1
fi
if ! printf '%s\n' "$symbols" | grep -q main; then
  echo "65C02 combined object is missing main" >&2
  printf '%s\n' "$symbols" >&2
  exit 1
fi
relocs=$("$ELFDUMP" -r "$WORK/combined65.o")
if ! printf '%s\n' "$relocs" | grep -q add; then
  echo "65C02 combined object dropped the relocation against add" >&2
  printf '%s\n' "$relocs" >&2
  exit 1
fi

echo "daveld relocatable tests passed"
