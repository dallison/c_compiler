#!/bin/bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 davecc" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
WORK="${TEST_TMPDIR:-/tmp}/6502-frame-push-helpers"
mkdir -p "$WORK"

# Optimized aggregate calls can push values directly from local-variable and
# argument frame slots. Verify that every helper emitted for those paths is
# provided by the modular 65C02 runtime archive.
cat >"$WORK/helper_refs.s" <<'SRC'
.text
.global frame_push_helper_refs
frame_push_helper_refs:
  JSR __push_var1
  JSR __push_var1b
  JSR __push_var2
  JSR __push_var2b
  JSR __push_var4
  JSR __push_var4b
  JSR __push_var8
  JSR __push_var8b
  JSR __push_arg1
  JSR __push_arg1b
  JSR __push_arg2
  JSR __push_arg2b
  JSR __push_arg4
  JSR __push_arg4b
  JSR __push_arg8
  JSR __push_arg8b
  RTS
SRC

"$DAVECC" -target 65c02 -nostdinc -static -Wl,-e -Wl,frame_push_helper_refs \
  "$WORK/helper_refs.s" -o "$WORK/helper_refs.exe"
