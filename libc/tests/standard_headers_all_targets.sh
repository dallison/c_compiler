#!/bin/bash
# Run the standard-header ledger for every supported compilation target.
set -uo pipefail

if [ "$#" -lt 1 ]; then
  echo "usage: $0 <davecc> [extra davecc flags...]" >&2
  exit 2
fi

davecc=$1
shift

if [ -f "libc/tests/standard_headers.sh" ]; then
  :
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/../.."
fi

failed=0
for target in x86_64 aarch64 arm riscv wasm32 pcode 65c02; do
  echo "== standard headers: $target =="
  if ! bash libc/tests/standard_headers.sh "$davecc" "$target" "$@"; then
    failed=1
  fi
done

exit "$failed"
