#!/bin/bash
set -uo pipefail

AARCH64="$1"
AARCH64_SIGNED="$2"
DYNAMIC="$3"
STATIC="$4"

cd "$(dirname "$DYNAMIC")"

run_expect() {
  local expected=$1
  shift
  set +e
  "$@"
  local status=$?
  set -e
  test "$status" -eq "$expected"
}

run_expect 99 "$AARCH64" -i hello_dynamic.exe
run_expect 99 env LD_BIND_NOW=1 "$AARCH64" -i hello_dynamic.exe
run_expect 99 env LD_BIND_NOW=1 "$AARCH64" -n hello_dynamic.exe
run_expect 42 "$AARCH64_SIGNED" -n "$STATIC"
