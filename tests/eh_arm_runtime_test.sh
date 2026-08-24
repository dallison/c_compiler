#!/usr/bin/env bash

set -euo pipefail

davecc=$1
interpreter=$2
libc=$3

if [[ -f "libc/tests/run_tests.sh" ]]; then
  :
elif [[ -n "${TEST_SRCDIR:-}" && -n "${TEST_WORKSPACE:-}" ]]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/.."
fi

work=${TEST_TMPDIR:-$(mktemp -d)}
mkdir -p "$work"
if [[ -z "${TEST_TMPDIR:-}" ]]; then
  trap 'rm -rf "$work"' EXIT
fi

cp tests/eh_lsda_runtime_test.sh "$work/run.sh"
bash "$work/run.sh" "$davecc" "$interpreter" "$libc" arm
