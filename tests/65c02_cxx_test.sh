#!/bin/bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 davecc interpreter rom source" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
ROM="$ROOT/$3"
SOURCE="$ROOT/$4"
WORK="${TEST_TMPDIR:-/tmp}/65c02-cxx"
mkdir -p "$WORK"

# The source dispatches through a secondary base and therefore requires a
# non-zero `this`-adjustor thunk. The driver must also infer static linkage for
# this static-only target.
"$DAVECC" -target 65c02 "$SOURCE" -o "$WORK/test.exe"
"$INTERPRETER" -rom "$ROM" "$WORK/test.exe"
