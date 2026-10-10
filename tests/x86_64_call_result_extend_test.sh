#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 davecc x86_64 source.c callee.s" >&2
  exit 2
fi

ROOT="${TEST_SRCDIR:-$(pwd)}/${TEST_WORKSPACE:-}"
DAVECC="$ROOT/$1"
INTERPRETER="$ROOT/$2"
SOURCE="$ROOT/$3"
CALLEE="$ROOT/$4"
WORK="${TEST_TMPDIR:-/tmp}/x86-64-call-result-extend"
mkdir -p "$WORK"

for opt in -O0 -O2; do
  "$DAVECC" -target x86_64 "$opt" -nostdinc -nostdlib -static \
    -Wl,-e -Wl,main "$SOURCE" "$CALLEE" -o "$WORK/test.exe"
  set +e
  "$INTERPRETER" -i "$WORK/test.exe"
  rc=$?
  set -e
  if [[ "$rc" -ne 0 ]]; then
    echo "x86-64 call result extension test ($opt) returned $rc; expected 0" >&2
    exit 1
  fi
done
