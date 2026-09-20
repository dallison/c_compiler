#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 davecc x86_64 source" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
SOURCE="$ROOT/$3"
WORK="${TEST_TMPDIR:-/tmp}/x86-64-six-arg-leaf"
mkdir -p "$WORK"

"$DAVECC" -target x86_64 -O1 -nostdinc -nostdlib -static \
  -Wl,-e -Wl,main "$SOURCE" -o "$WORK/test.exe"

set +e
"$INTERPRETER" -i "$WORK/test.exe"
rc=$?
set -e
if [[ "$rc" -ne 0 ]]; then
  echo "x86-64 six-arg leaf test returned $rc; expected 0" >&2
  exit 1
fi
