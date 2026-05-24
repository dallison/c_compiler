#!/bin/bash
# Bazel entry point for libc/tests/run_tests.sh
set -euo pipefail

if [ "$#" -lt 3 ]; then
  echo "usage: $0 <mode> <davecc> <target> [interpreter] [libc_archive]" >&2
  echo "  mode: all | compiler | compile-all | runtime" >&2
  exit 2
fi

mode=$1
davecc=$2
target=$3
interpreter=${4:-}
libc_archive=${5:-}

if [ -f "libc/tests/run_tests.sh" ]; then
  :
elif [ -n "${TEST_SRCDIR:-}" ] && [ -n "${TEST_WORKSPACE:-}" ]; then
  cd "${TEST_SRCDIR}/${TEST_WORKSPACE}"
else
  cd "$(dirname "$0")/../.."
fi

if [ -n "$libc_archive" ]; then
  export LIBC_ARCHIVE="$libc_archive"
fi

case "$mode" in
  all)
    if [ -z "$interpreter" ]; then
      echo "runtime tests require an interpreter path" >&2
      exit 2
    fi
    exec bash libc/tests/run_tests.sh --compiler --runtime \
      "$davecc" "$target" "$interpreter"
    ;;
  compiler)
    exec bash libc/tests/run_tests.sh --compiler "$davecc" "$target"
    ;;
  compile-all)
    exec bash libc/tests/run_tests.sh --compile-all "$davecc" "$target"
    ;;
  runtime)
    if [ -z "$interpreter" ]; then
      echo "runtime tests require an interpreter path" >&2
      exit 2
    fi
    exec bash libc/tests/run_tests.sh --runtime "$davecc" "$target" "$interpreter"
    ;;
  *)
    echo "unknown mode: $mode" >&2
    exit 2
    ;;
esac
