#!/usr/bin/env bash
set -euo pipefail

while [[ $# -gt 0 ]]; do
  case "$1" in
    --davecc) davecc="$2"; shift 2 ;;
    --libc) libc="$2"; shift 2 ;;
    --interpreter) interpreter="$2"; shift 2 ;;
    --elfdump) elfdump="$2"; shift 2 ;;
    --sources) sources="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

: "${davecc:?}"
: "${libc:?}"
: "${interpreter:?}"
: "${elfdump:?}"
: "${sources:?}"
sources=$(dirname "$sources")
export DAVECC_INCLUDE_DIR="${DAVECC_INCLUDE_DIR:-$(dirname "$davecc")/libc/include}"

work=$(mktemp -d "${TMPDIR:-/tmp}/davecc-init-fini-dso.XXXXXX")
trap 'rm -rf "$work"' EXIT

"$davecc" -target x86_64 -shared "$sources/dep.c" "$libc" -o "$work/dep.so"
"$davecc" -target x86_64 -shared -rpath "$work" "$sources/dso.c" \
  "$work/dep.so" -o "$work/dso.so"
"$davecc" -target x86_64 -Wl,-e -Wl,main -rpath "$work" \
  "$sources/main.c" "$work/dso.so" "$libc" -o "$work/test_dynamic.bin"

for artifact in "$work/dep.so" "$work/dso.so" "$work/test_dynamic.bin"; do
  if [[ ! -f "$artifact" ]]; then
    echo "missing linked artifact: $artifact" >&2
    exit 1
  fi
done

dynamic=$("$elfdump" -d "$work/test_dynamic.bin")
case "$dynamic" in
  *"FINI_ARRAY"*) ;;
  *) echo "dynamic executable is missing DT_FINI_ARRAY" >&2; exit 1 ;;
esac

if [[ "$(uname -s)" == "Darwin" ]]; then
  echo "skipping dynamic DSO execution on Darwin: fixed-address MAP_FIXED is unsupported here" >&2
  exit 0
fi

(
  cd "$work"
  set +e
  LD_LIBRARY_PATH="$work:${LD_LIBRARY_PATH:-}" "$interpreter" -i test_dynamic.bin
  rc=$?
  set -e
  if [[ "$rc" -ne 0 ]]; then
    echo "expected interpreter exit 0 after DSO fini reached state 6, got $rc" >&2
    exit 1
  fi
)
