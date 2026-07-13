#!/usr/bin/env bash
set -euo pipefail

while [[ $# -gt 0 ]]; do
  case "$1" in
    --davecc) davecc="$2"; shift 2 ;;
    --archivist) archivist="$2"; shift 2 ;;
    --libc) libc="$2"; shift 2 ;;
    --interpreter) interpreter="$2"; shift 2 ;;
    --elfdump) elfdump="$2"; shift 2 ;;
    --sources) sources="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

: "${davecc:?}"
: "${archivist:?}"
: "${libc:?}"
: "${interpreter:?}"
: "${elfdump:?}"
: "${sources:?}"
sources=$(dirname "$sources")

work=$(mktemp -d "${TMPDIR:-/tmp}/davecc-init-fini.XXXXXX")
trap 'rm -rf "$work"' EXIT

"$davecc" -target x86_64 -std=c++20 -c "$sources/a.cpp" -o "$work/a.o"
"$davecc" -target x86_64 -std=c++20 -c "$sources/b.cpp" -o "$work/b.o"
"$davecc" -target x86_64 -std=c++20 -c "$sources/main.cpp" -o "$work/main.o"
"$davecc" -target x86_64 -c "$sources/foreign_arrays.s" \
  -o "$work/foreign_arrays.o"
"$archivist" r "$work/libfirst.a" "$work/a.o"
"$davecc" -target x86_64 -static -Wl,-e -Wl,main \
  "$work/foreign_arrays.o" "$work/libfirst.a" "$work/b.o" "$work/main.o" \
  "$libc" -o "$work/test.bin"

sections=$("$elfdump" -S "$work/test.bin")
case "$sections" in
  *".init_array"*"init_array"*) ;;
  *) echo "linked executable is missing typed .init_array" >&2; exit 1 ;;
esac

"$interpreter" -i "$work/test.bin"
