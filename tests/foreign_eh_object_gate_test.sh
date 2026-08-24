#!/bin/bash
set -euo pipefail

DAVECC="$1"
INTERPRETER="$2"
LIBC="$3"
DAVECC_SOURCE="$4"
FOREIGN_SOURCE="$5"
TARGET="$6"
CLANG_TARGET="$7"
shift 7

WORK="${TEST_TMPDIR:-$(mktemp -d "${TMPDIR:-/tmp}/foreign-eh-object.XXXXXX")}"
mkdir -p "$WORK"

CLANG_CXX="${CLANG_CXX:-clang++}"
ARCHIVIST="${ARCHIVIST:-$(dirname "$DAVECC")/archivist}"
if ! command -v "$CLANG_CXX" >/dev/null 2>&1; then
  echo "missing host clang++ for foreign EH object gate" >&2
  exit 1
fi
if [[ ! -x "$ARCHIVIST" ]]; then
  echo "missing archivist for foreign EH archive gate" >&2
  exit 1
fi

foreign_cxxflags=("-fno-pic" "-fomit-frame-pointer")
if [[ "$TARGET" == "x86_64" ]]; then
  # DaveCC's x86-64 image is linked above 4 GiB, so foreign objects must not
  # use the small code model's 32-bit absolute address relocations.
  foreign_cxxflags+=("-mcmodel=large")
fi

"$DAVECC" -target "$TARGET" -std=c++20 -O1 -fexceptions \
  -isystem libc/include -c "$DAVECC_SOURCE" -o "$WORK/davecc.o"
"$CLANG_CXX" --target="$CLANG_TARGET" -std=c++20 -O1 -fexceptions \
  "${foreign_cxxflags[@]}" \
  -c "$FOREIGN_SOURCE" \
  -o "$WORK/foreign.o"

"$DAVECC" -target "$TARGET" -std=c++20 -O1 -fexceptions -static \
  -isystem libc/include -Wl,-e -Wl,main \
  "$WORK/davecc.o" "$WORK/foreign.o" "$LIBC" -o "$WORK/interop.exe"

"$INTERPRETER" "$@" "$WORK/interop.exe"

"$ARCHIVIST" r "$WORK/libdavecc_eh.a" "$WORK/davecc.o"
"$DAVECC" -target "$TARGET" -std=c++20 -O1 -fexceptions -static \
  -isystem libc/include -Wl,-e -Wl,main \
  "$WORK/foreign.o" "$WORK/libdavecc_eh.a" "$LIBC" \
  -o "$WORK/interop_davecc_archive.exe"
"$INTERPRETER" "$@" "$WORK/interop_davecc_archive.exe"

"$ARCHIVIST" r "$WORK/libforeign_eh.a" "$WORK/foreign.o"
"$DAVECC" -target "$TARGET" -std=c++20 -O1 -fexceptions -static \
  -isystem libc/include -Wl,-e -Wl,main \
  "$WORK/davecc.o" "$WORK/libforeign_eh.a" "$LIBC" \
  -o "$WORK/interop_foreign_archive.exe"
"$INTERPRETER" "$@" "$WORK/interop_foreign_archive.exe"
